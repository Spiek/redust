#include "redisinterface.h"

RedisInterface::RedisInterface(QString list, QString connectionName)
{
    this->redisList = list.toLocal8Bit();
    this->connectionName = connectionName;
}


//
// Key-Value Redis Functions
//

void RedisInterface::del(bool async)
{
    // Build and execute Command
    // We use DEL command to delete the whole HASH list
    // src: http://redis.io/commands/del
    QByteArray res;
    RedisInterface::execRedisCommand({"DEL", this->redisList }, async ? 0 : &res);
}

bool RedisInterface::exists()
{
    // Build and execute Command
    // EXISTS list
    // src: http://redis.io/commands/exists
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "EXISTS", this->redisList }, &returnValue);

    // return result
    return returnValue == "1";
}


//
// Hash Redis Functions
//

int RedisInterface::hlen()
{
    // Build and execute Command
    // HLEN list
    // src: http://redis.io/commands/hlen
    QByteArray res;
    RedisInterface::execRedisCommand({"HLEN", this->redisList }, &res);
    return res.toInt();
}

bool RedisInterface::hset(QByteArray key, QByteArray value, bool waitForAnswer)
{
    // Build and execute Command
    // HSET list key value
    // src: http://redis.io/commands/hset
    QByteArray returnValue;
    bool result = RedisInterface::execRedisCommand({ "HSET", this->redisList, key, value }, waitForAnswer ? &returnValue : 0);

    // determinate result
    if(!waitForAnswer) return result;
    else return result && returnValue == "OK";
}

bool RedisInterface::hexists(QByteArray key)
{
    // Build and execute Command
    // HEXISTS list key
    // src: http://redis.io/commands/hexists
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "HEXISTS", this->redisList, key }, &returnValue);

    // return result
    return returnValue == "1";
}

bool RedisInterface::hdel(QByteArray key, bool waitForAnswer)
{
    // Build and execute Command
    // HDEL list key
    // src: http://redis.io/commands/hdel
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "HDEL", this->redisList, key}, waitForAnswer ? &returnValue : 0);

    // return result
    return waitForAnswer ? returnValue == "1" : true;
}

QByteArray RedisInterface::hget(QByteArray key)
{
    // Build and execute Command
    // HGET list key
    // src: http://redis.io/commands/hget
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "HGET", this->redisList, key }, &returnValue);

    // return result
    return returnValue;
}

void RedisInterface::hkeys(QList<QByteArray>& result)
{
    // Build and execute Command
    // HKEYS list
    // src: http://redis.io/commands/hkeys
    RedisInterface::execRedisCommand({ "HKEYS", this->redisList }, 0, &result);
}

void RedisInterface::hvals(QList<QByteArray> &result)
{
    // Build and execute Command
    // HVALS list
    // src: http://redis.io/commands/hvals
    RedisInterface::execRedisCommand({ "HVALS", this->redisList }, 0, &result);
}

void RedisInterface::hgetall(QList<QByteArray>& result)
{
    // Build and execute Command
    // HGETALL list
    // src: http://redis.io/commands/hgetall
    RedisInterface::execRedisCommand({ "HGETALL", this->redisList }, 0, &result);
}

void RedisInterface::hgetall(QMap<QByteArray, QByteArray>& result)
{
    // get data
    QList<QByteArray> elements;
    this->hgetall(elements);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end(); itr += 2) {
        result.insert(*itr, *(itr + 1));
    }
}

void RedisInterface::hgetall(QHash<QByteArray, QByteArray>& result)
{
    // get data
    QList<QByteArray> elements;
    this->hgetall(elements);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end(); itr += 2) {
        result.insert(*itr, *(itr + 1));
    }
}


//
// Scan Redis Functions
//

void RedisInterface::scan(QList<QByteArray>* resultKeys, QList<QByteArray>* resultValues, int count, int pos, int* newPos)
{
    return this->simplifyHScan(0, resultKeys, resultValues, 0, count, pos, newPos);
}

void RedisInterface::scan(QList<QByteArray>& keyValues, int count, int pos, int* newPos)
{
    return this->simplifyHScan(&keyValues, 0, 0, 0, count, pos, newPos);
}

void RedisInterface::scan(QMap<QByteArray, QByteArray>& keyValues, int count, int pos, int* newPos)
{
    return this->simplifyHScan(0, 0, 0, &keyValues, count, pos, newPos);
}


//
// command simplifier
//

void RedisInterface::simplifyHScan(QList<QByteArray> *lstKeyValues, QList<QByteArray>* keys, QList<QByteArray>* values, QMap<QByteArray, QByteArray>* keyValues, int count, int pos, int* newPos)
{
    // if caller don't want any data, exit
    if(!lstKeyValues && !keys && !values && !keyValues) return;

    // Build and exec following command
    // HSCAN list cursor COUNT count
    // src: http://redis.io/commands/scan
    QList<QByteArray> type;
    QList<QByteArray> elements;
    if(RedisInterface::execRedisCommand({"HSCAN", this->redisList, QString::number(pos).toLocal8Bit(), "COUNT", QString::number(count).toLocal8Bit() }, 0, &type, &elements)) {
        // set new pos if wanted
        if(newPos && !type.isEmpty()) *newPos = atoi(type.first().data());

        // copy elements to given data
        for(auto itr = elements.begin(); itr != elements.end();) {
            // save key if wanted
            if(keys) keys->append(*itr);
            if(lstKeyValues) lstKeyValues->append(*itr);
            itr++;

            // save value if wanted
            if(values) values->append(*itr);
            if(lstKeyValues) lstKeyValues->append(*itr);

            // save keyvalues
            if(keyValues) keyValues->insert(*(itr - 1), *itr);
            itr++;
        }
    }
}


//
// helper
//

bool RedisInterface::execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result, QList<QByteArray>* lstResultArray1, QList<QByteArray>* lstResultArray2)
{
    // acquire socket
    bool waitForAnswer = result || lstResultArray1 || lstResultArray2;
    QTcpSocket* socket = RedisConnectionManager::requestConnection(this->connectionName, !waitForAnswer);
    if(!socket) return false;

    /// Build and execute RESP request
    /// see: http://redis.io/topics/protocol#resp-arrays
    // 1. determine RESP request packet size
    int size = 15;
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) size += 15 + itr->length();

    // 2. build RESP request
    char contentValue[size];
    char* content = contentValue;
    content += sprintf(content, "*%i\r\n", cmd.size());
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) {
        content += sprintf(content, "$%i\r\n", itr->isEmpty() ? -1 : itr->length());
        content = (char*)mempcpy(content, itr->data(), itr->length());
        *content = '\r';
        *++content = '\n';
        content++;
    }

    // 3. exec RESP request
    socket->write(contentValue, content - contentValue);

#ifdef Q_OS_WIN
    // Write Bug Windows workaround:
    // in some test cases a del command was not sent to the redis server over the event loop, to fix this behavior force writing the RESP Request to the redis server, every time
    //
    // The Qt documentation said:
    // Note: This function may fail randomly on Windows. Consider using the event loop and the bytesWritten() signal if your software will run on Windows.
    // src: http://doc.qt.io/qt-5/qabstractsocket.html#waitForBytesWritten
    if(!waitForAnswer) while(!socket->waitForBytesWritten());
#endif

    // exit if we don't have to parse the return code
    if(!waitForAnswer) return true;

    // 4. wait for server return code
    socket->waitForReadyRead();
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
        while((!segmentLength && !(protoSegmentEnd = strstr(*rawData, "\n"))) || (segmentLength && data.length() - (*rawData - data.data()) < segmentLength)) {
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

    // handle Simple String, Error and Integer
    if(result && (respDataType == '+' || respDataType == '-' || respDataType == ':')) {
        // get pointer to end of current segment and be sure we have enough data available to read
        char* protoSegmentNext = readSegement(&rawData);

        // read segment (but without the segment end chars \r and \n)
        *result = QByteArray(rawData, protoSegmentNext - rawData - 2);
        rawData = protoSegmentNext;
        return true;
    }

    // handle Bulk String
    if(result && respDataType == '$') {
        // 1. get string length and be sure we have enough data to read
        char* protoSegmentNext = readSegement(&rawData);
        int length = atoi(rawData);
        rawData = protoSegmentNext;

        // 2. get whole string of previous parsed length and be sure we have enough data to read
        if(length == -1) {
            *result = QByteArray();
        } else {
            protoSegmentNext = readSegement(&rawData, length + 2);
            *result = QByteArray(rawData, length);
            rawData = protoSegmentNext;
        }
        return true;
    }

    // handle Array(s)
    if(respDataType == '*') {
        rawData--;
        QList<QByteArray>* currentArray = 0;
        int elementCount = 0;

        // loop until we have parsed all segments
        while(true) {
            // read segment
            char* protoSegmentNext = readSegement(&rawData);

            // parse packet header
            char packetType = *rawData++;
            int length = atoi(rawData);
            rawData = protoSegmentNext;

            // if we have a collection packet
            if(packetType == '*') {
                elementCount = length + 1;
                if(!currentArray) currentArray = lstResultArray1;
                else currentArray = lstResultArray2;

                // stop parsing if data is not wanted by the caller
                if(!currentArray) break;
            }

            // if we have a null bulk string, so create a null byte array
            else if(length == -1) {
                currentArray->append(QByteArray());
            }

            // otherwise we have normal bulk string, so parse it
            else {
                protoSegmentNext = readSegement(&rawData, length + 2);
                currentArray->append(QByteArray(rawData, length));
                rawData = protoSegmentNext;
            }

            // if we have no elements left, exit
            if(!--elementCount) break;
        }
        return true;
    }

    // otherwise we have an parse error
    return false;
}
