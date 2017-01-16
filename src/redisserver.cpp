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
                                 type == RequestType::Syncron        ?  RedisServer::ConnectionType::Blocked :
                                                                        RedisServer::ConnectionType::ReadWrite;
        socket = this->requestConnection(conType);
    }

    // check socket
    RedisServer::RedisRequest request(new RedisRequestData(type, socket));
    request->cmd(cmd.front());
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
    else if(socket != this->socketReadWrite && (type == RequestType::Asyncron || type == RequestType::PipeLine)) {
        qWarning("Executions of %s-Requests are only supported on RedisServer's own ReadWrite Socket, request may not handled correctly...", type == RequestType::Asyncron ? "Asyncron" : "Pipeline");
    }
    else if(type == RequestType::WriteOnlyBlocked || type == RequestType::Syncron) {
        if(!socket->waitForBytesWritten()) request->error("Write Wait Error");
        if(type == RequestType::Syncron && !this->parseResponse(request)) {
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

    // some command based normalizations
    // [H|S|Z|]SCAN
    // - move 1. element of 1. arraylist element to cursor and 2. arraylist element to array
    if(request->cmd() == "SCAN" ||
       request->cmd() == "HSCAN" ||
       request->cmd() == "SSCAN" ||
       request->cmd() == "ZSCAN")
    {
        response->cursor(response->arrayList().front().front().toInt());
        response->array(response->arrayList().back());
        response->arrayList().clear();
    }

    // everything okay
    return true;
}

int RedisServer::executePipeline(bool waitForBytesWritten)
{
    if(this->pendingPipelineRequests.isEmpty()) return 0;
    int count = this->pendingPipelineData.count();
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

RedisServer::RedisRequest RedisServer::del(QByteArray key, RequestType type)
{
    // Build and execute Command
    // DEL List
    // src: http://redis.io/commands/del
    return this->execRedisCommand({"DEL", key }, type);
}

RedisServer::RedisRequest RedisServer::exists(QByteArray key, RequestType type)
{
    // Build and execute Command
    // EXISTS list
    // src: http://redis.io/commands/exists
    return this->execRedisCommand({"EXISTS", key }, type);
}

RedisServer::RedisRequest RedisServer::keys(QByteArray pattern, RequestType type)
{
    // Build and execute Command
    // KEYS pattern
    // src: http://redis.io/commands/KEYS
    return this->execRedisCommand({"KEYS", pattern }, type);
}

RedisServer::RedisRequest RedisServer::lpush(QByteArray key, QByteArray value, RequestType type)
{
    return RedisServer::lpush(key, std::list<QByteArray>{value}, type);
}

RedisServer::RedisRequest RedisServer::lpush(QByteArray key, std::list<QByteArray> values, RequestType type)
{
    // Build and execute Command
    // LPUSH key value [value]...
    // src: http://redis.io/commands/lpush
    values.push_front(key);
    values.push_front("LPUSH");

    // exec async
    return this->execRedisCommand(values, type);
}

RedisServer::RedisRequest RedisServer::rpush(QByteArray key, QByteArray value, RequestType type)
{
    return RedisServer::rpush(key, std::list<QByteArray>{value}, type);
}

RedisServer::RedisRequest RedisServer::rpush(QByteArray key, std::list<QByteArray> values, RequestType type)
{
    // Build and execute Command
    // RPUSH key value [value]...
    // src: http://redis.io/commands/lpush
    values.push_front(key);
    values.push_front("RPUSH");

    // exec async
    return this->execRedisCommand(values, type);
}

bool RedisServer::blpop(QTcpSocket *socket, std::list<QByteArray> lists, int timeout, RequestType type)
{
    // Build and execute Command
    // src: http://redis.io/commands/BLPOP lists timeout
    lists.push_front("BLPOP");
    lists.push_back(QByteArray::number(timeout));
    return !this->execRedisCommand(lists, type, socket)->hasError();
}

bool RedisServer::brpop(QTcpSocket *socket, std::list<QByteArray> lists, int timeout, RequestType type)
{
    // Build and execute Command
    // src: http://redis.io/commands/BRPOP lists timeout
    lists.push_front("BRPOP");
    lists.push_back(QByteArray::number(timeout));
    return !this->execRedisCommand(lists, type, socket)->hasError();
}

RedisServer::RedisRequest RedisServer::llen(QByteArray key, RequestType type)
{
    // Build and execute Command
    // LLEN key
    // src: http://redis.io/commands/llen
    return this->execRedisCommand({"LLEN", key}, type);
}

RedisServer::RedisRequest RedisServer::hlen(QByteArray list, RequestType type)
{
    // Build and execute Command
    // HLEN list
    // src: http://redis.io/commands/hlen
    return this->execRedisCommand({"HLEN", list}, type);
}

RedisServer::RedisRequest RedisServer::hset(QByteArray list, QByteArray key, QByteArray value, RequestType type)
{
    // Build and execute Command
    // HSET list key value
    // src: http://redis.io/commands/hset
    return this->execRedisCommand({ "HSET", list, key, value }, type);
}

RedisServer::RedisRequest RedisServer::hsetnx(QByteArray list, QByteArray key, QByteArray value, RequestType type)
{
    // Build and execute Command
    // HSETNX list key value
    // src: http://redis.io/commands/hsetnx
    return this->execRedisCommand({ "HSETNX", list, key, value }, type);
}

RedisServer::RedisRequest RedisServer::hmset(QByteArray list, std::list<QByteArray> keys, std::list<QByteArray> values, RequestType type)
{
    // Build and execute Command
    // HMSET list key value [ key value ] ...
    // src: http://redis.io/commands/hmset
    if(keys.size() != values.size()) return RedisServer::RedisRequest(new RedisRequestData(type, "key/value size is different!"));
    std::list<QByteArray> lstCmd = { "HMSET", list };
    auto itrKey = keys.begin();
    auto itrValue = values.begin();
    for(;itrKey != keys.end();) {
        lstCmd.push_back(*itrKey++);
        lstCmd.push_back(*itrValue++);
    }

    // execute
    return this->execRedisCommand(lstCmd, type);
}

RedisServer::RedisRequest RedisServer::hmset(QByteArray list, std::map<QByteArray, QByteArray> entries, RequestType type)
{
    // Build and execute Command
    // HMSET list key value [ key value ] ...
    // src: http://redis.io/commands/hmset
    std::list<QByteArray> lstCmd = { "HMSET", list };
    for(auto itr = entries.begin(); itr != entries.end(); itr++) {
        lstCmd.push_back(itr->first);
        lstCmd.push_back(itr->second);
    }

    // execute
    return this->execRedisCommand(lstCmd, type);
}

RedisServer::RedisRequest RedisServer::hexists(QByteArray list, QByteArray key, RequestType type)
{
    // Build and execute Command
    // HEXISTS list key
    // src: http://redis.io/commands/hexists
    return this->execRedisCommand({ "HEXISTS", list, key }, type);
}

RedisServer::RedisRequest RedisServer::hdel(QByteArray list, QByteArray key, RequestType type)
{
    // Build and execute Command
    // HDEL list key
    // src: http://redis.io/commands/hdel
    return this->execRedisCommand({ "HDEL", list, key}, type);
}

RedisServer::RedisRequest RedisServer::hget(QByteArray list, QByteArray key, RequestType type)
{
    // Build and execute Command
    // HGET list key
    // src: http://redis.io/commands/hget
    return this->execRedisCommand({ "HGET", list, key }, type);
}

RedisServer::RedisRequest RedisServer::hgetall(QByteArray list, RequestType type)
{
    // Build and execute Command
    // HGETALL list
    // src: http://redis.io/commands/hgetall
    return this->execRedisCommand({ "HGETALL", list}, type);
}

RedisServer::RedisRequest RedisServer::hmget(QByteArray list, std::list<QByteArray> keys, RequestType type)
{
    // Build and execute Command
    // HMGET list [ key ] ...
    // src: http://redis.io/commands/hmget
    keys.push_front(list);
    keys.push_front("HMGET");
    return this->execRedisCommand(keys, type);
}

RedisServer::RedisRequest RedisServer::hstrlen(QByteArray list, QByteArray key, RequestType type)
{
    // Build and execute Command
    // HSTRLEN key field
    // src: http://redis.io/commands/hstrlen
    return this->execRedisCommand({ "HSTRLEN", list, key }, type);
}

RedisServer::RedisRequest RedisServer::hkeys(QByteArray list, RequestType type)
{
    // Build and execute Command
    // HKEYS list
    // src: http://redis.io/commands/hkeys
    return this->execRedisCommand({ "HKEYS", list }, type);
}

RedisServer::RedisRequest RedisServer::hvals(QByteArray list, RequestType type)
{
    // Build and execute Command
    // HVALS list
    // src: http://redis.io/commands/hvals
    return this->execRedisCommand({ "HVALS", list }, type);
}

RedisServer::RedisRequest RedisServer::scan(QByteArray cursor, int count, QByteArray pattern, RequestType type)
{
    return this->scan("SCAN", "", cursor, count, pattern, type);
}

RedisServer::RedisRequest RedisServer::sscan(QByteArray key, QByteArray cursor, int count, QByteArray pattern, RequestType type)
{
    return this->scan("SSCAN", key, cursor, count, pattern, type);
}

RedisServer::RedisRequest RedisServer::hscan(QByteArray key, QByteArray cursor, int count, QByteArray pattern, RequestType type)
{
    return this->scan("HSCAN", key, cursor, count, pattern, type);
}

RedisServer::RedisRequest RedisServer::zscan(QByteArray key, QByteArray cursor, int count, QByteArray pattern, RequestType type)
{
    return this->scan("ZSCAN", key, cursor, count, pattern, type);
}

RedisServer::RedisRequest RedisServer::scan(QByteArray scanType, QByteArray key, QByteArray cursor, int count, QByteArray pattern, RequestType type)
{
    // Build and execute Command
    // [|S|H|Z]SCAN cursor [MATCH pattern] [COUNT count]
    // src: http://redis.io/commands/scan
    std::list<QByteArray> lstCmd = { scanType };
    if(!key.isEmpty()) {
        lstCmd.push_back(key);
    }
    lstCmd.push_back(cursor);
    if(!pattern.isEmpty()) {
        lstCmd.push_back("MATCH");
        lstCmd.push_back(pattern);
    }
    if(count != -1) {
        lstCmd.push_back("COUNT");
        lstCmd.push_back(QByteArray::number(count));
    }
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

