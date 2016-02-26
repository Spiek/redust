#include "recotec/redisinterface.h"

//
// General Redis Functions
//

bool RedisInterface::ping(RedisServer &server, QByteArray data, bool async)
{
    // Build and execute Command
    // PING [data]
    // src: http://redis.io/commands/ping
    QByteArray res;
    QList<QByteArray> lstCmd = { "PING" };
    if(!data.isEmpty()) lstCmd.append(data);
    bool result = RedisInterface::execRedisCommand(server, lstCmd, async ? 0 : &res);

    // evaluate result
    if(async) return result;
    else return data.isEmpty() ? res == "PONG" : res == data;
}


//
// Key-Value Redis Functions
//

bool RedisInterface::del(RedisServer& server, QByteArray key, bool async)
{
    // Build and execute Command
    // DEL List
    // src: http://redis.io/commands/del
    QByteArray res;
    bool result = RedisInterface::execRedisCommand(server, {"DEL", key }, async ? 0 : &res);

    if(async) return result;
    else return result && res.toInt() == 1;
}

bool RedisInterface::exists(RedisServer& server, QByteArray key)
{
    // Build and execute Command
    // EXISTS list
    // src: http://redis.io/commands/exists
    QByteArray returnValue;
    RedisInterface::execRedisCommand(server, { "EXISTS", key }, &returnValue);

    // return result
    return returnValue == "1";
}

QList<QByteArray> RedisInterface::keys(RedisServer& server, QByteArray pattern)
{
    // Build and execute Command
    // KEYS pattern
    // src: http://redis.io/commands/KEYS
    QList<QByteArray> keys;
    RedisInterface::execRedisCommand(server, { "KEYS", pattern }, 0, &keys);
    return keys;
}


//
// List Redis Functions
//
bool RedisInterface::bpop(QTcpSocket* socket, QList<QByteArray> lists, RedisInterface::PopDirection direction, int timeout)
{
    // Build and execute Command
    // src: http://redis.io/commands/[BLPOP|BRPOP] lists timeout
    lists.prepend(direction == RedisInterface::PopDirection::Begin ? "BLPOP" : "BRPOP");
    lists.append(QByteArray::number(timeout));
    return RedisInterface::execRedisCommand(socket, lists, 0, 0, 0, false);
}



//
// Hash Redis Functions
//

int RedisInterface::hlen(RedisServer& server, QByteArray list)
{
    // Build and execute Command
    // HLEN list
    // src: http://redis.io/commands/hlen
    QByteArray res;
    RedisInterface::execRedisCommand(server, {"HLEN", list }, &res);
    return res.toInt();
}

bool RedisInterface::hset(RedisServer& server, QByteArray list, QByteArray key, QByteArray value, bool waitForAnswer, bool onlySetIfNotExists)
{
    // Build and execute Command
    // HSET/HSETNX list key value
    // src: http://redis.io/commands/hset
    QByteArray returnValue;
    return RedisInterface::execRedisCommand(server, { onlySetIfNotExists ? "HSETNX" : "HSET", list, key, value }, waitForAnswer ? &returnValue : 0);
}

bool RedisInterface::hmset(RedisServer& server, QByteArray list, QList<QByteArray> keys, QList<QByteArray> values, bool waitForAnswer)
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
    return RedisInterface::execRedisCommand(server, lst, waitForAnswer ? &returnValue : 0);
}

bool RedisInterface::hexists(RedisServer& server, QByteArray list, QByteArray key)
{
    // Build and execute Command
    // HEXISTS list key
    // src: http://redis.io/commands/hexists
    QByteArray returnValue;
    RedisInterface::execRedisCommand(server, { "HEXISTS", list, key }, &returnValue);

    // return result
    return returnValue == "1";
}

bool RedisInterface::hdel(RedisServer& server, QByteArray list, QByteArray key, bool waitForAnswer)
{
    // Build and execute Command
    // HDEL list key
    // src: http://redis.io/commands/hdel
    QByteArray returnValue;
    RedisInterface::execRedisCommand(server, { "HDEL", list, key}, waitForAnswer ? &returnValue : 0);

    // return result
    return waitForAnswer ? returnValue == "1" : true;
}

QByteArray RedisInterface::hget(RedisServer& server, QByteArray list, QByteArray key)
{
    // Build and execute Command
    // HGET list key
    // src: http://redis.io/commands/hget
    QByteArray returnValue;
    RedisInterface::execRedisCommand(server, { "HGET", list, key }, &returnValue);

    // return result
    return returnValue;
}

QList<QByteArray> RedisInterface::hmget(RedisServer& server, QByteArray list, QList<QByteArray> keys)
{
    // Build and execute Command
    // HMGET list key [ key ] ...
    // src: http://redis.io/commands/hmget
    QList<QByteArray> returnValue;
    keys.prepend(list);
    keys.prepend("HMGET");
    RedisInterface::execRedisCommand(server, keys, 0, &returnValue);
    return returnValue;
}

int RedisInterface::hstrlen(RedisServer& server, QByteArray list, QByteArray key)
{
    // Build and execute Command
    // HSTRLEN key field
    // src: http://redis.io/commands/hstrlen
    QByteArray returnValue;

    // return -2 if command is not known by redis and return -1 on general error
    if(!RedisInterface::execRedisCommand(server, { "HSTRLEN", list, key }, &returnValue)) {
        if(returnValue.startsWith("ERR unknown command")) return -2;
        return -1;
    }

    // return converted value length
    return returnValue.toInt();
}

void RedisInterface::hkeys(RedisServer& server, QByteArray list, QList<QByteArray>& result)
{
    // Build and execute Command
    // HKEYS list
    // src: http://redis.io/commands/hkeys
    RedisInterface::execRedisCommand(server, { "HKEYS", list }, 0, &result);
}

void RedisInterface::hvals(RedisServer& server, QByteArray list, QList<QByteArray> &result)
{
    // Build and execute Command
    // HVALS list
    // src: http://redis.io/commands/hvals
    RedisInterface::execRedisCommand(server, { "HVALS", list }, 0, &result);
}

void RedisInterface::hgetall(RedisServer& server, QByteArray list, QList<QByteArray>& result)
{
    // Build and execute Command
    // HGETALL list
    // src: http://redis.io/commands/hgetall
    RedisInterface::execRedisCommand(server, { "HGETALL", list }, 0, &result);
}

void RedisInterface::hgetall(RedisServer& server, QByteArray list, QMap<QByteArray, QByteArray>& result)
{
    // get data
    QList<QByteArray> elements;
    RedisInterface::hgetall(server, list, elements);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end(); itr += 2) {
        result.insert(*itr, *(itr + 1));
    }
}

void RedisInterface::hgetall(RedisServer& server, QByteArray list, QHash<QByteArray, QByteArray>& result)
{
    // get data
    QList<QByteArray> elements;
    RedisInterface::hgetall(server, list, elements);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end(); itr += 2) {
        result.insert(*itr, *(itr + 1));
    }
}


//
// Scan Redis Functions
//

void RedisInterface::scan(RedisServer& server, QByteArray list, QList<QByteArray>* resultKeys, QList<QByteArray>* resultValues, int count, int pos, int* newPos)
{
    return RedisInterface::simplifyHScan(server, list, 0, resultKeys, resultValues, 0, count, pos, newPos);
}

void RedisInterface::scan(RedisServer& server, QByteArray list, QList<QByteArray>& keyValues, int count, int pos, int* newPos)
{
    return RedisInterface::simplifyHScan(server, list, &keyValues, 0, 0, 0, count, pos, newPos);
}

void RedisInterface::scan(RedisServer& server, QByteArray list, QMap<QByteArray, QByteArray>& keyValues, int count, int pos, int* newPos)
{
    return RedisInterface::simplifyHScan(server, list, 0, 0, 0, &keyValues, count, pos, newPos);
}


//
// command simplifier
//

void RedisInterface::simplifyHScan(RedisServer& server, QByteArray list, QList<QByteArray> *lstKeyValues, QList<QByteArray>* keys, QList<QByteArray>* values, QMap<QByteArray, QByteArray>* keyValues, int count, int pos, int* newPos)
{
    // if caller don't want any data, exit
    if(!lstKeyValues && !keys && !values && !keyValues) return;

    // Build and exec following command
    // HSCAN list cursor COUNT count
    // src: http://redis.io/commands/scan
    QList<QList<QByteArray>> returnValue;
    if(!RedisInterface::execRedisCommand(server, {"HSCAN", list, QString::number(pos).toLocal8Bit(), "COUNT", QString::number(count).toLocal8Bit() }, 0, 0, &returnValue) ||
       returnValue.length() != 2) {
            return;
    }

    // set new pos if wanted
    QByteArray posScan = returnValue.takeFirst().value(0);
    if(newPos) *newPos = atoi(posScan.data());

    // copy elements to given data
    QList<QByteArray>& elements = returnValue.first();
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


//
// Private helpers
//
bool RedisInterface::execRedisCommand(RedisServer& server, QList<QByteArray> cmd, QByteArray* result, QList<QByteArray>* resultArray, QList<QList<QByteArray> > *result2dArray)
{
    bool waitForAnswer = result || resultArray || result2dArray;
    QTcpSocket* socket = server.requestConnection(waitForAnswer ? RedisServer::ConnectionType::ReadWrite :
                                                                  RedisServer::ConnectionType::WriteOnly);
    return RedisInterface::execRedisCommand(socket, cmd, result, resultArray, result2dArray, waitForAnswer);
}

bool RedisInterface::execRedisCommand(QTcpSocket *socket, QList<QByteArray> cmd, QByteArray* result, QList<QByteArray>* resultArray, QList<QList<QByteArray> > *result2dArray, bool waitForAnswer)
{
    // check socket
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
        content = (char*)mempcpy(content, "\r\n", 2);
    }

    // 3. write RESP request to socket
    socket->write(contentOriginPos, content - contentOriginPos);
    free(contentOriginPos);

    // if we don't have to wait for an answer return true, otherwise parse return value and return the parsing result
    return !waitForAnswer ? true : RedisInterface::parseResponse(socket, result, resultArray, result2dArray);
}


//
// Public helpers
//

bool RedisInterface::parseResponse(QTcpSocket* socket, QByteArray *result, QList<QByteArray> *resultArray, QList<QList<QByteArray> > *result2dArray)
{
    // check params
    if(!socket || !socket->isReadable() || (!result && !resultArray && !result2dArray)) return false;

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
        bool firstRun = true;
        do {
            // read segment
            char* protoSegmentNext = readSegement(&rawData);

            // parse packet header
            char packetType = *rawData++;
            int length = atoi(rawData);
            rawData = protoSegmentNext;

            // if we have a collection packet
            if(packetType == '*') {
                elementCount = length + 1;

                // if resultArray is present, save first array in resultArray and all others (if present) in result2dArray
                // if no result Array is present and result2dArray is present, create a new array element in it
                currentArray = resultArray && firstRun ? resultArray :
                               result2dArray ? &*result2dArray->insert(result2dArray->end(), QList<QByteArray>()) : 0;

                // handle null multi bulk by creating single null value in array and exit loop
                if(firstRun && length == -1) {
                    if(currentArray) currentArray->append(QByteArray());
                    break;
                }
                if(firstRun) firstRun = false;
            }

            // if we have a null bulk string, so create a null byte array
            else if(length == -1) {
                if(currentArray) currentArray->append(QByteArray());
            }

            // otherwise we have normal bulk string, so parse it
            else {
                protoSegmentNext = readSegement(&rawData, length + 2);
                if(currentArray) currentArray->append(QByteArray(rawData, length));
                rawData = protoSegmentNext;
            }
        } while(--elementCount);
        return true;
    }

    // otherwise we have an parse error
    return false;
}
