#include "redismapprivate.h"

RedisMapPrivate::RedisMapPrivate(QString list, QString connectionName)
{
    this->redisList = list.toLocal8Bit();
    this->connectionName = connectionName;
}

void RedisMapPrivate::clear(bool async)
{
    // Build and execute Command
    // There is currently no implementation of HDEL MYLIST so we executing a script on the server doing the job
    // src: http://redis.io/commands/del#comment-1006084933 (adapted to hash)
    QByteArray res;
    RedisMapPrivate::execRedisCommand({"eval", "for _,k in ipairs(redis.call('HKEYS','" + this->redisList + "')) do redis.call('HDEL', '" + this->redisList + "', k) end", "0" }, async ? 0 : &res);
}

bool RedisMapPrivate::insert(QByteArray key, QByteArray value, bool waitForAnswer)
{
    // Build and execute Command
    // HSET list key value
    // src: http://redis.io/commands/HSET
    QByteArray returnValue;
    bool result = RedisMapPrivate::execRedisCommand({ "HSET", this->redisList, key, value }, waitForAnswer ? &returnValue : 0);

    // determinate result
    if(!waitForAnswer) return result;
    else return result && returnValue == "OK";
}

QByteArray RedisMapPrivate::value(QByteArray key)
{
    // Build and execute Command
    // HGET list key
    // src: http://redis.io/commands/HGET
    QByteArray returnValue;
    RedisMapPrivate::execRedisCommand({ "HGET", this->redisList, key }, &returnValue);

    // return result
    return returnValue;
}

QList<QByteArray> RedisMapPrivate::keys(int count, int pos, int* newPos)
{
    // if the caller want all keys, we are using HKEYS
    if(count <= 0) {
        QList<QByteArray> elements;
        RedisMapPrivate::execRedisCommand({ "HKEYS", this->redisList }, 0, &elements);
        return elements;
    }

    // otherwise we iterate over the keys using SCAN
    return this->simplifyHScan(count, pos, true, false, newPos);
}

QList<QByteArray> RedisMapPrivate::values(int count, int pos, int* newPos)
{
    // if the caller want all keys, we are using HVALS
    if(count <= 0) {
        QList<QByteArray> elements;
        RedisMapPrivate::execRedisCommand({ "HVALS", this->redisList }, 0, &elements);
        return elements;
    }

    // otherwise we iterate over the values using SCAN
    return this->simplifyHScan(count, pos, false, true, newPos);
}

bool RedisMapPrivate::execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result, QList<QByteArray>* lstResultArray1, QList<QByteArray>* lstResultArray2)
{
    // acquire socket
    bool waitForAnswer = result || lstResultArray1 || lstResultArray2;
    QTcpSocket* socket = RedisConnectionManager::requestConnection(this->connectionName, !waitForAnswer);
    if(!socket) return false;

    /// Build and execute RESP request
    /// see: http://redis.io/topics/protocol#resp-arrays
    // 1. determine allocation size
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
                elementCount = length;
                if(!currentArray) currentArray = lstResultArray1;
                else currentArray = lstResultArray2;

                // stop parsing if data is not wanted by the caller
                if(!currentArray) break;
                continue;
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


QList<QByteArray> RedisMapPrivate::simplifyHScan(int count, int pos, bool key, bool value, int *newPos)
{
    // if caller don't want a key or a value return an empty list
    if(!key && !value) return QList<QByteArray>();

    // Build and exec following command
    // HSCAN list cursor COUNT count
    // src: http://redis.io/commands/scan
    QList<QByteArray> type;
    QList<QByteArray> elements;
    if(RedisMapPrivate::execRedisCommand({"HSCAN", this->redisList, QString::number(pos).toLocal8Bit(), "COUNT", QString::number(count).toLocal8Bit() }, 0, &type, &elements)) {
        // set new pos if wanted
        if(newPos && !type.isEmpty()) *newPos = atoi(type.first().data());

        // delete keys or values if wanted
        if(!key || !value) {
            int iElement = 0;
            for(auto itr = elements.begin(); itr != elements.end();) {
                if(++iElement % 2 == (key ? 0 : 1)) itr = elements.erase(itr);
                else itr++;
            }
        }
    }

    // return generated elements
    return elements;
}
