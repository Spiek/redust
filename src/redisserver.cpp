#include "redust/redisserver.h"

RedisServer::RedisServer(QString redisServer, qint16 redisPort)
{
    this->strRedisConnectionHost = redisServer;
    this->intRedisConnectionPort = redisPort;
}

RedisServer::~RedisServer()
{
    delete this->socketWriteOnly;
    delete this->socketReadWrite;
    qDeleteAll(this->lstBlockedSockets);
}

bool RedisServer::initConnections(bool readWrite, bool writeOnly, int blockedSockets)
{
    // acquire write socket, and exit on fail
    if(writeOnly && !this->requestConnection(RedisServer::ConnectionType::WriteOnly)) return false;

    // acquire readwrite socket, and exit on fail
    if(readWrite && !this->requestConnection(RedisServer::ConnectionType::ReadWrite)) return false;

    // acquire readwrite blockedSockets sockets, and exit on fail
    for(int i = blockedSockets - this->lstBlockedSockets.count(); i > 0; i--) {
        if(!this->requestConnection(RedisServer::ConnectionType::Blocked)) return false;
    }

    // all successfull constructed
    return true;
}

QTcpSocket* RedisServer::requestConnection(RedisServer::ConnectionType type)
{
    // acquire blocked socket
    QTcpSocket* socket = 0;
    if(type == ConnectionType::Blocked) {
        if(!this->lstBlockedSockets.isEmpty()) return this->lstBlockedSockets.dequeue();
        socket = new QTcpSocket;
    }

    // acquire write socket
    else if(type == ConnectionType::WriteOnly) {
        if(this->socketWriteOnly) return socketWriteOnly;
        socket = this->socketWriteOnly = new QTcpSocket;
    }

    // acquire read/write socket
    else if(type == ConnectionType::ReadWrite) {
        if(this->socketReadWrite) return socketReadWrite;
        socket = this->socketReadWrite = new QTcpSocket;
        QObject::connect(this->socketReadWrite, &QTcpSocket::readyRead, this, &RedisServer::handleRedisResponse);
    }

    // if we have no socket, exit
    if(!socket) return 0;

    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // socket not connected at this point, so let us blocked syncronly connect
    socket->connectToHost(this->strRedisConnectionHost, this->intRedisConnectionPort, type == ConnectionType::WriteOnly ? QIODevice::WriteOnly : QIODevice::ReadWrite);
    if(!socket->waitForConnected(5000)) {
        qFatal("Cannot connect to Redis Server %s:%i, give up...", qPrintable(this->strRedisConnectionHost), this->intRedisConnectionPort);

        // cleanup blocked socket
        if(type == ConnectionType::Blocked) delete socket;
        return 0;
    }

    // socket successfull connected
    return socket;
}

void RedisServer::freeBlockedConnection(QTcpSocket *socket)
{
    // append socket to blocked connection list
    if(socket) this->lstBlockedSockets.append(socket);
}

RedisServer::RedisRequest RedisServer::execRedisCommand(std::list<QByteArray> cmd, RequestType type, QTcpSocket* socket)
{
    // if socket is not available, try to acquire socket by RequestType
    if(!socket) {
        ConnectionType conType = type == RequestType::WriteOnly      ?  RedisServer::ConnectionType::WriteOnly :
                                 type == RequestType::BlockedSyncron ?  RedisServer::ConnectionType::Blocked :
                                                                        RedisServer::ConnectionType::ReadWrite;
        socket = this->requestConnection(conType);
    }

    // check socket
    RedisServer::RedisRequest request(new RedisRequestData(socket));
    if(!socket) {
        request->error("No Socket");
        return request;
    }

    /// Build and execute RESP request
    /// see: http://redis.io/topics/protocol#resp-arrays
    // 1. determine RESP request packet size
    int size = 15;
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) size += 15 + itr->length();

    // 2. build RESP request
    char contentOriginPos[size];
    char* content = contentOriginPos;

    // build packet
    *content++ = '*';
    itoa(cmd.size(), content, 10);
    content += this->numIntPlaces(cmd.size());
    content = (char*)mempcpy(content, "\r\n", 2);
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) {
        *content++ = '$';
        itoa(itr->isNull() ? -1 : itr->length(), content, 10);
        content += (int)itr->isNull() ? 2 : itr->isEmpty() ? 1 : this->numIntPlaces(itr->length());
        content = (char*)mempcpy(content, "\r\n", 2);
        if(!itr->isNull()) {
            content = (char*)mempcpy(content, itr->data(), itr->length());
            content = (char*)mempcpy(content, "\r\n", 2);
        }
    }

    // 3. write RESP request to socket (and exit on error)
    if(type != RequestType::PipeLine && socket->write(QByteArray::fromRawData(contentOriginPos, content - contentOriginPos)) == -1) {
        request->error("Write Error");
        return request;
    }

    // 4. handle types
    if(type == RequestType::WriteOnly);
    else if(type == RequestType::WriteOnlyBlocked || type == RequestType::BlockedSyncron) {
        if(!socket->waitForBytesWritten()) request->error("Write Wait Error");
        if(type == RequestType::BlockedSyncron && !this->parseResponse(request)) {
            request->error("Redis Parse Error");
        }
    } else if(type == RequestType::Asyncron) {
        this->pendingRequests.enqueue(request);
    } else if(type == RequestType::PipeLine) {
        this->pendingPipelineRequests.enqueue(request);
        this->pendingPipelineData.append(QByteArray::fromRawData(contentOriginPos, content - contentOriginPos));
    }

    // return response
    return request;
}

bool RedisServer::parseResponse(RedisServer::RedisRequest& request)
{
    // build response
    QTcpSocket* socket = request->socket();
    RedisResponse response = request->response();

    // some error checks
    if(!socket) {
        response->error("Not Socket");
        return false;
    } else if(!socket->isReadable()) {
        response->error("Not Readable");
        return false;
    }

    // get data (wait syncronly if no data is available)
    if(!socket->bytesAvailable()) socket->waitForReadyRead();
    QByteArray data = socket->readAll();

    /// Parse RESP Response
    /// see: http://redis.io/topics/protocol#resp-protocol-description

    // readSegement
    // - be sure enough data is available to read the next protocol segment
    //   if not enough data is given remove allready parsed data and realloc the rawData to point to the new data
    // returns: a pointer to the next char after the end of the segment
    // Note: if no segmentLength is given (segmentLength = 0), the end of the next segment is determined by searching for the next '\n' and returns the position after that char
    //       if segmentLength is given, the end of the segment is the next char after segmentLength chars
    auto readSegement = [&socket, &data](char** rawData, int segmentLength = 0) {
        // loop until we have enough data to parse the next segment
        char* protoSegmentEnd = 0;
        while((!segmentLength && !(protoSegmentEnd = strstr(*rawData, "\n"))) || (segmentLength && data.size() - (*rawData - data.data()) < segmentLength)) {
            // remove allready parsed data from cache
            data.remove(0, *rawData - data.data());

            // if no data is present so we wait for it
            if(!socket->bytesAvailable()) socket->waitForReadyRead();

            // read all available data and update the raw pointer
            data += socket->readAll();
            *rawData = data.data();
        }
        return !segmentLength ? ++protoSegmentEnd : *rawData + segmentLength;
    };

    // simplify variables
    char* rawData = data.data();
    char respDataType = *rawData++;

    // determine type
    response->type(respDataType == '+' ? RedisResponseData::Type::String :
                   respDataType == '-' ? RedisResponseData::Type::Error :
                   respDataType == ':' ? RedisResponseData::Type::Integer :
                   respDataType == '$' ? RedisResponseData::Type::String :
                   respDataType == '*' ? RedisResponseData::Type::Array : RedisResponseData::Type::ArrayList);

    // handle BulkString
    if(respDataType == '$') {
        // get string length and be sure we have enough data to read
        char* protoSegmentNext = readSegement(&rawData);
        int length = atoi(rawData);
        rawData = protoSegmentNext;

        // get whole string of previous parsed length and be sure we have enough data to read
        QByteArray segmentData;
        if(length != -1) {
            protoSegmentNext = readSegement(&rawData, length + 2);
            segmentData = QByteArray(rawData, length);
            rawData = protoSegmentNext;
        }

        // construct RedisString
        response->string(segmentData);
    }

    // handle base types (SimpleString, Error and Integer)
    else if(response->type() == RedisResponseData::Type::String ||
       response->type() == RedisResponseData::Type::Error ||
       response->type() == RedisResponseData::Type::Integer)
    {
        // get pointer to end of current segment and be sure we have enough data available to read
        char* protoSegmentNext = readSegement(&rawData);

        // read segment (but without the segment end chars \r and \n)
        QByteArray segment = QByteArray(rawData, protoSegmentNext - rawData - 2);
        rawData = protoSegmentNext;

        // set data to redis structs
        if(response->type() == RedisResponseData::Type::String)  response->string(segment);
        if(response->type() == RedisResponseData::Type::Error)   response->error(QString(segment));
        if(response->type() == RedisResponseData::Type::Integer) response->integer(segment.toInt());
    }
    // handle Arrays and Multi Bulk Arrays
    else if(response->type() == RedisResponseData::Type::Array) {
        rawData--;

        // parse array
        std::list<QByteArray>* currentArray = 0;
        int currentElementCount = 0;
        int allElementsCount = 0;

        // parse array elements
        do {
            // read segment
            char* protoSegmentNext = readSegement(&rawData);

            // parse packet header
            char packetType = *rawData++;
            int length = atoi(rawData);
            rawData = protoSegmentNext;

            // handle array type
            if(packetType == '*') {
                if(allElementsCount > 0) allElementsCount--;
                currentElementCount = length == -1 ? 1 : length + 1;
                allElementsCount += currentElementCount;

                currentArray = &*response->arrayList().insert(--response->arrayList().begin(), std::list<QByteArray>());

                // handle null multi bulk by creating single null value in array and exit loop
                if(length == -1) currentArray->push_back(QByteArray());
            }

            // if we have a null bulk string, so create a null byte array
            else if(length == -1) {
                currentArray->push_back(QByteArray());
            }

            // otherwise we have normal bulk string, so parse it
            else {
                protoSegmentNext = readSegement(&rawData, length + 2);
                currentArray->push_back(QByteArray(rawData, length));
                rawData = protoSegmentNext;
            }
            if(currentElementCount == 1 && allElementsCount > 1) {
                currentArray = &*--response->arrayList().end();
                currentElementCount = allElementsCount;
            }
        } while(--currentElementCount && --allElementsCount);

        // if we have less or equal one item, return a RedisArray otherwise a RedisArrayList
        if(response->arrayList().size() > 1) response->type(RedisResponseData::Type::ArrayList);
        else if(response->arrayList().size() == 1) {
            response->array(*response->arrayList().begin());
            response->arrayList().clear();
        }
    }

    // write not parsed data back to socket
    for(auto itr = data.end() - 1; itr != rawData - 1; itr--) {
        socket->ungetChar(*itr);
    }

    // everything okay
    return true;
}

int RedisServer::executePipeline(bool waitForBytesWritten)
{
    if(this->pendingPipelineRequests.isEmpty()) return 0;
    int count = this->pendingPipelineRequests.count();
    this->pendingRequests.append(this->pendingPipelineRequests);
    this->socketReadWrite->write(this->pendingPipelineData);
    this->pendingPipelineRequests.clear();
    this->pendingPipelineData.clear();
    if(waitForBytesWritten) this->socketReadWrite->waitForBytesWritten();
    return count;
}

RedisServer::RedisRequest RedisServer::ping(QByteArray data, RequestType type)
{
    // Build and execute Command
    // PING [data]
    // src: http://redis.io/commands/ping
    std::list<QByteArray> lstCmd = { "PING" };
    if(!data.isEmpty()) lstCmd.push_back(data);

    return this->execRedisCommand(lstCmd, type);
}

void RedisServer::handleRedisResponse()
{
    if(!this->socketReadWrite) return;
    while(this->socketReadWrite->bytesAvailable()) {
        if(this->pendingRequests.isEmpty()) qDebug() << "No pending Requests available!!";
        RedisServer::RedisRequest request = this->pendingRequests.dequeue();
        bool result = this->parseResponse(request);
        this->redisResponseFinished(request, result);
    }
}

