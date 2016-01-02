#include "recotec/redisinterface.h"

//
// General Redis Functions
//

bool RedisInterface::ping(QByteArray data, bool async, QString connectionPool)
{
    // Build and execute Command
    // PING [data]
    // src: http://redis.io/commands/ping
    QByteArray res;
    QList<QByteArray> lstCmd = { "PING" };
    if(!data.isEmpty()) lstCmd.append(data);
    bool result = RedisInterface::execRedisCommand(lstCmd, connectionPool, async ? 0 : &res);

    // evaluate result
    if(async) return result;
    else return data.isEmpty() ? res == "PONG" : res == data;
}


//
// Key-Value Redis Functions
//

bool RedisInterface::del(QByteArray key, bool async, QString connectionPool)
{
    // Build and execute Command
    // DEL List
    // src: http://redis.io/commands/del
    QByteArray res;
    bool result = RedisInterface::execRedisCommand({"DEL", key }, connectionPool, async ? 0 : &res);

    if(async) return result;
    else return result && res.toInt() == 1;
}

bool RedisInterface::exists(QByteArray key, QString connectionPool)
{
    // Build and execute Command
    // EXISTS list
    // src: http://redis.io/commands/exists
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "EXISTS", key }, connectionPool, &returnValue);

    // return result
    return returnValue == "1";
}

QList<QByteArray> RedisInterface::keys(QByteArray pattern, QString connectionPool)
{
    // Build and execute Command
    // KEYS pattern
    // src: http://redis.io/commands/KEYS
    QList<QByteArray> keys;
    RedisInterface::execRedisCommand({ "KEYS", pattern }, connectionPool, 0, &keys);
    return keys;
}


//
// Hash Redis Functions
//

int RedisInterface::hlen(QByteArray list, QString connectionPool)
{
    // Build and execute Command
    // HLEN list
    // src: http://redis.io/commands/hlen
    QByteArray res;
    RedisInterface::execRedisCommand({"HLEN", list }, connectionPool, &res);
    return res.toInt();
}

bool RedisInterface::hset(QByteArray list, QByteArray key, QByteArray value, bool waitForAnswer, bool onlySetIfNotExists, QString connectionPool)
{
    // Build and execute Command
    // HSET/HSETNX list key value
    // src: http://redis.io/commands/hset
    QByteArray returnValue;
    return RedisInterface::execRedisCommand({ onlySetIfNotExists ? "HSETNX" : "HSET", list, key, value }, connectionPool, waitForAnswer ? &returnValue : 0);
}

bool RedisInterface::hmset(QByteArray list, QList<QByteArray> keys, QList<QByteArray> values, bool waitForAnswer, QString connectionPool)
{
    // Build and execute Command
    // HMSET list key value [ key value ] ...
    // src: http://redis.io/commands/hmset
    if(keys.length() != values.length()) return false;
    QByteArray returnValue;
    QList<QByteArray> lst = { "HMSET", list };
    auto itrKey = keys.begin();
    auto itrValue = values.begin();
    for(;itrKey != keys.end();) {
        lst << *itrKey++ << *itrValue++;
    }
    return RedisInterface::execRedisCommand(lst, connectionPool, waitForAnswer ? &returnValue : 0);
}

bool RedisInterface::hexists(QByteArray list, QByteArray key, QString connectionPool)
{
    // Build and execute Command
    // HEXISTS list key
    // src: http://redis.io/commands/hexists
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "HEXISTS", list, key }, connectionPool, &returnValue);

    // return result
    return returnValue == "1";
}

bool RedisInterface::hdel(QByteArray list, QByteArray key, bool waitForAnswer, QString connectionPool)
{
    // Build and execute Command
    // HDEL list key
    // src: http://redis.io/commands/hdel
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "HDEL", list, key}, connectionPool, waitForAnswer ? &returnValue : 0);

    // return result
    return waitForAnswer ? returnValue == "1" : true;
}

QByteArray RedisInterface::hget(QByteArray list, QByteArray key, QString connectionPool)
{
    // Build and execute Command
    // HGET list key
    // src: http://redis.io/commands/hget
    QByteArray returnValue;
    RedisInterface::execRedisCommand({ "HGET", list, key }, connectionPool, &returnValue);

    // return result
    return returnValue;
}

QList<QByteArray> RedisInterface::hmget(QByteArray list, QList<QByteArray> keys, QString connectionPool)
{
    // Build and execute Command
    // HMGET list key [ key ] ...
    // src: http://redis.io/commands/hmget
    QList<QByteArray> returnValue;
    keys.prepend(list);
    keys.prepend("HMGET");
    RedisInterface::execRedisCommand(keys, connectionPool, 0, &returnValue);
    return returnValue;
}

void RedisInterface::hkeys(QByteArray list, QList<QByteArray>& result, QString connectionPool)
{
    // Build and execute Command
    // HKEYS list
    // src: http://redis.io/commands/hkeys
    RedisInterface::execRedisCommand({ "HKEYS", list }, connectionPool, 0, &result);
}

void RedisInterface::hvals(QByteArray list, QList<QByteArray> &result, QString connectionPool)
{
    // Build and execute Command
    // HVALS list
    // src: http://redis.io/commands/hvals
    RedisInterface::execRedisCommand({ "HVALS", list }, connectionPool, 0, &result);
}

void RedisInterface::hgetall(QByteArray list, QList<QByteArray>& result, QString connectionPool)
{
    // Build and execute Command
    // HGETALL list
    // src: http://redis.io/commands/hgetall
    RedisInterface::execRedisCommand({ "HGETALL", list }, connectionPool, 0, &result);
}

void RedisInterface::hgetall(QByteArray list, QMap<QByteArray, QByteArray>& result, QString connectionPool)
{
    // get data
    QList<QByteArray> elements;
    RedisInterface::hgetall(list, elements, connectionPool);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end(); itr += 2) {
        result.insert(*itr, *(itr + 1));
    }
}

void RedisInterface::hgetall(QByteArray list, QHash<QByteArray, QByteArray>& result, QString connectionPool)
{
    // get data
    QList<QByteArray> elements;
    RedisInterface::hgetall(list, elements, connectionPool);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end(); itr += 2) {
        result.insert(*itr, *(itr + 1));
    }
}


//
// Scan Redis Functions
//

void RedisInterface::scan(QByteArray list, QList<QByteArray>* resultKeys, QList<QByteArray>* resultValues, int count, int pos, int* newPos, QString connectionPool)
{
    return RedisInterface::simplifyHScan(list, 0, resultKeys, resultValues, 0, count, pos, newPos, connectionPool);
}

void RedisInterface::scan(QByteArray list, QList<QByteArray>& keyValues, int count, int pos, int* newPos, QString connectionPool)
{
    return RedisInterface::simplifyHScan(list, &keyValues, 0, 0, 0, count, pos, newPos, connectionPool);
}

void RedisInterface::scan(QByteArray list, QMap<QByteArray, QByteArray>& keyValues, int count, int pos, int* newPos, QString connectionPool)
{
    return RedisInterface::simplifyHScan(list, 0, 0, 0, &keyValues, count, pos, newPos, connectionPool);
}


//
// command simplifier
//

void RedisInterface::simplifyHScan(QByteArray list, QList<QByteArray> *lstKeyValues, QList<QByteArray>* keys, QList<QByteArray>* values, QMap<QByteArray, QByteArray>* keyValues, int count, int pos, int* newPos, QString connectionPool)
{
    // if caller don't want any data, exit
    if(!lstKeyValues && !keys && !values && !keyValues) return;

    // Build and exec following command
    // HSCAN list cursor COUNT count
    // src: http://redis.io/commands/scan
    QList<QByteArray> type;
    QList<QByteArray> elements;
    if(RedisInterface::execRedisCommand({"HSCAN", list, QString::number(pos).toLocal8Bit(), "COUNT", QString::number(count).toLocal8Bit() }, connectionPool, 0, &type, &elements)) {
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

bool RedisInterface::execRedisCommand(QList<QByteArray> cmd, QString connectionPool, QByteArray* result, QList<QByteArray>* lstResultArray1, QList<QByteArray>* lstResultArray2)
{
    // acquire socket
    bool waitForAnswer = result || lstResultArray1 || lstResultArray2;
    QTcpSocket* socket = RedisConnectionManager::requestConnection(connectionPool, !waitForAnswer);
    if(!socket) return false;

    /// Build and execute RESP request
    /// see: http://redis.io/topics/protocol#resp-arrays
    // 1. determine RESP request packet size
    int size = 15;
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) size += 15 + itr->length();

    // 2. build RESP request
    char* contentOriginPos = (char*)malloc(size);
    char* content = contentOriginPos;
    content += sprintf(content, "*%i\r\n", cmd.size());
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) {
        content += sprintf(content, "$%i\r\n", itr->isEmpty() ? -1 : itr->length());
        content = (char*)mempcpy(content, itr->data(), itr->length());
        *content = '\r';
        *++content = '\n';
        content++;
    }

    // 3. exec RESP request
    socket->write(contentOriginPos, content - contentOriginPos);
    free(contentOriginPos);

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

        // return false on error type, otherwise true
        return respDataType != '-';
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
