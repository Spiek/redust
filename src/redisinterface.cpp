#include "recotec/redisinterface.h"

//
// General Redis Functions
//

bool RedisInterface::ping(RedisServer &server, QByteArray data, bool async)
{
    // Build and execute Command
    // PING [data]
    // src: http://redis.io/commands/ping
    std::list<QByteArray> lstCmd = { "PING" };
    if(!data.isEmpty()) lstCmd.push_back(data);

    // exec async
    if(async) return RedisInterface::execRedisCommand(server, lstCmd, true);

    // exec syncron and evaluate result
    RedisInterface::RedisData redisResult = RedisInterface::execRedisCommandResult(server, lstCmd);
    return data.isEmpty() ? redisResult.string == "PONG" : redisResult.string == data;
}


//
// Key-Value Redis Functions
//

bool RedisInterface::del(RedisServer& server, QByteArray key, bool async)
{
    // Build and execute Command
    // DEL List
    // src: http://redis.io/commands/del
    std::list<QByteArray> lstCmd = {"DEL", key };

    // exec async
    if(async) return RedisInterface::execRedisCommand(server, lstCmd, true);

    // exec syncron and evaluate result
    return RedisInterface::execRedisCommandResult(server, lstCmd).integer == 1;
}

bool RedisInterface::exists(RedisServer& server, QByteArray key)
{
    // Build and execute Command
    // EXISTS list
    // src: http://redis.io/commands/exists
    return RedisInterface::execRedisCommandResult(server, { "EXISTS", key }).integer == 1;
}

std::list<QByteArray> RedisInterface::keys(RedisServer& server, QByteArray pattern)
{
    // Build and execute Command
    // KEYS pattern
    // src: http://redis.io/commands/KEYS
    return RedisInterface::execRedisCommandResult(server, { "KEYS", pattern }).array;
}


//
// List Redis Functions
//
int RedisInterface::push(RedisServer &server, QByteArray key, QByteArray value, RedisInterface::Position direction, bool waitForAnswer)
{
    return RedisInterface::push(server, key, std::list<QByteArray>{value}, direction, waitForAnswer);
}

int RedisInterface::push(RedisServer &server, QByteArray key, std::list<QByteArray> values, RedisInterface::Position direction, bool waitForAnswer)
{
    // Build and execute Command
    // [L|R]PUSH key value [value]...
    // src: http://redis.io/commands/lpush
    values.push_front(key);
    values.push_front(direction == RedisInterface::Position::Begin ? "LPUSH" : "RPUSH");

    // exec syncron
    if(waitForAnswer) return RedisInterface::execRedisCommandResult(server, values).integer;

    // exec async
    RedisInterface::execRedisCommand(server, values, true);
    return -1;
}

bool RedisInterface::bpop(QTcpSocket* socket, std::list<QByteArray> lists, RedisInterface::Position direction, int timeout)
{
    // Build and execute Command
    // src: http://redis.io/commands/[BLPOP|BRPOP] lists timeout
    lists.push_front(direction == RedisInterface::Position::Begin ? "BLPOP" : "BRPOP");
    lists.push_back(QByteArray::number(timeout));
    return RedisInterface::execRedisCommand(socket, lists);
}

int RedisInterface::llen(RedisServer &server, QByteArray key)
{
    // Build and execute Command
    // LLEN key
    // src: http://redis.io/commands/llen
    return RedisInterface::execRedisCommandResult(server, {"LLEN", key}).integer;
}


//
// Hash Redis Functions
//

int RedisInterface::hlen(RedisServer& server, QByteArray list)
{
    // Build and execute Command
    // HLEN list
    // src: http://redis.io/commands/hlen
    return RedisInterface::execRedisCommandResult(server, {"HLEN", list}).integer;
}

bool RedisInterface::hset(RedisServer& server, QByteArray list, QByteArray key, QByteArray value, bool waitForAnswer, bool onlySetIfNotExists)
{
    // Build and execute Command
    // HSET/HSETNX list key value
    // src: http://redis.io/commands/hset
    std::list<QByteArray> lstCmd = { onlySetIfNotExists ? "HSETNX" : "HSET", list, key, value };

    // execute
    return waitForAnswer ? RedisInterface::execRedisCommandResult(server, lstCmd).integer >= 0 :
                           RedisInterface::execRedisCommand(server, lstCmd, true);
}

bool RedisInterface::hmset(RedisServer& server, QByteArray list, std::list<QByteArray> keys, std::list<QByteArray> values, bool waitForAnswer)
{
    // Build and execute Command
    // HMSET list key value [ key value ] ...
    // src: http://redis.io/commands/hmset
    if(keys.size() != values.size()) return false;
    std::list<QByteArray> lstCmd = { "HMSET", list };
    auto itrKey = keys.begin();
    auto itrValue = values.begin();
    for(;itrKey != keys.end();) {
        lstCmd.push_back(*itrKey++);
        lstCmd.push_back(*itrValue++);
    }

    // execute
    return waitForAnswer ? RedisInterface::execRedisCommandResult(server, lstCmd).string == "OK" :
                           RedisInterface::execRedisCommand(server, lstCmd, true);
}

bool RedisInterface::hexists(RedisServer& server, QByteArray list, QByteArray key)
{
    // Build and execute Command
    // HEXISTS list key
    // src: http://redis.io/commands/hexists
    return RedisInterface::execRedisCommandResult(server, { "HEXISTS", list, key }).integer == 1;
}

bool RedisInterface::hdel(RedisServer& server, QByteArray list, QByteArray key, bool waitForAnswer)
{
    // Build and execute Command
    // HDEL list key
    // src: http://redis.io/commands/hdel
    std::list<QByteArray> lstCmd = { "HDEL", list, key};

    // execute
    return waitForAnswer ? RedisInterface::execRedisCommandResult(server, lstCmd).integer == 1 :
                           RedisInterface::execRedisCommand(server, lstCmd, true);
}

QByteArray RedisInterface::hget(RedisServer& server, QByteArray list, QByteArray key)
{
    // Build and execute Command
    // HGET list key
    // src: http://redis.io/commands/hget
    return RedisInterface::execRedisCommandResult(server, { "HGET", list, key }).string;
}

std::list<QByteArray> RedisInterface::hmget(RedisServer& server, QByteArray list, std::list<QByteArray> keys)
{
    // Build and execute Command
    // HMGET list key [ key ] ...
    // src: http://redis.io/commands/hmget
    keys.push_front(list);
    keys.push_front("HMGET");
    return RedisInterface::execRedisCommandResult(server, keys).array;
}

int RedisInterface::hstrlen(RedisServer& server, QByteArray list, QByteArray key)
{
    // Build and execute Command
    // HSTRLEN key field
    // src: http://redis.io/commands/hstrlen
    RedisInterface::RedisData redisResult = RedisInterface::execRedisCommandResult(server, { "HSTRLEN", list, key });

    // return -2 if command is not known by redis and return -1 on general error otherwise just the integer
    if(!redisResult.errorString.isEmpty()) {
        if(redisResult.errorString.startsWith("ERR unknown command")) return -2;
        else return -1;
    }
    return redisResult.integer;
}

void RedisInterface::hkeys(RedisServer& server, QByteArray list, std::list<QByteArray>& result)
{
    // FIXME: result should be returned!
    // Build and execute Command
    // HKEYS list
    // src: http://redis.io/commands/hkeys
    result = RedisInterface::execRedisCommandResult(server, { "HKEYS", list }).array;
}

void RedisInterface::hvals(RedisServer& server, QByteArray list, std::list<QByteArray> &result)
{
    // FIXME: result should be returned!
    // Build and execute Command
    // HVALS list
    // src: http://redis.io/commands/hvals
    result = RedisInterface::execRedisCommandResult(server, { "HVALS", list }).array;
}

void RedisInterface::hgetall(RedisServer& server, QByteArray list, std::list<QByteArray>& result)
{
    // FIXME: result should be returned!
    // Build and execute Command
    // HGETALL list
    // src: http://redis.io/commands/hgetall
    result = RedisInterface::execRedisCommandResult(server, { "HGETALL", list }).array;
}

void RedisInterface::hgetall(RedisServer& server, QByteArray list, QMap<QByteArray, QByteArray>& result)
{
    // get data
    std::list<QByteArray> elements;
    RedisInterface::hgetall(server, list, elements);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end();) {
        QByteArray key = *itr++;
        QByteArray value = *itr++;
        result.insert(key, value);
    }
}

void RedisInterface::hgetall(RedisServer& server, QByteArray list, QHash<QByteArray, QByteArray>& result)
{
    // get data
    std::list<QByteArray> elements;
    RedisInterface::hgetall(server, list, elements);

    // copy elements to qmap
    for(auto itr = elements.begin(); itr != elements.end();) {
        QByteArray key = *itr++;
        QByteArray value = *itr++;
        result.insert(key, value);
    }
}


//
// Scan Redis Functions
//

void RedisInterface::scan(RedisServer& server, QByteArray list, std::list<QByteArray>* resultKeys, std::list<QByteArray>* resultValues, int count, int pos, int* newPos)
{
    return RedisInterface::simplifyHScan(server, list, 0, resultKeys, resultValues, 0, count, pos, newPos);
}

void RedisInterface::scan(RedisServer& server, QByteArray list, std::list<QByteArray>& keyValues, int count, int pos, int* newPos)
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

void RedisInterface::simplifyHScan(RedisServer& server, QByteArray list, std::list<QByteArray> *lstKeyValues, std::list<QByteArray>* keys, std::list<QByteArray>* values, QMap<QByteArray, QByteArray>* keyValues, int count, int pos, int* newPos)
{
    // if caller don't want any data, exit
    if(!lstKeyValues && !keys && !values && !keyValues) return;

    // Build and exec command
    // HSCAN list cursor COUNT count
    // src: http://redis.io/commands/scan
    RedisInterface::RedisData redisResult = RedisInterface::execRedisCommandResult(server, {"HSCAN", list, QString::number(pos).toLocal8Bit(), "COUNT", QString::number(count).toLocal8Bit() });

    // exit on error
    if(!redisResult.errorString.isEmpty() || redisResult.arrayList.size() != 2)  return;
    std::list<std::list<QByteArray>>& returnValue = redisResult.arrayList;

    // set new pos if wanted
    QByteArray posScan = returnValue.front().front();
    returnValue.pop_front();
    if(newPos) *newPos = atoi(posScan.data());

    // copy elements to given data
    std::list<QByteArray>& elements = returnValue.front();
    for(auto itr = elements.begin(); itr != elements.end();) {
        // simplify key value
        QByteArray key = *itr++;
        QByteArray value = *itr++;

        // save key if wanted
        if(keys) keys->push_back(key);
        if(lstKeyValues) lstKeyValues->push_back(key);

        // save value if wanted
        if(values) values->push_back(value);
        if(lstKeyValues) lstKeyValues->push_back(value);

        // save keyvalues
        if(keyValues) keyValues->insert(key, value);
    }
}


//
// Redis Command Execution Helper
//
RedisInterface::RedisData RedisInterface::execRedisCommandResult(RedisServer& server, std::list<QByteArray> cmd)
{
    // acquire socket
    QTcpSocket* socket = server.requestConnection(RedisServer::ConnectionType::ReadWrite);

    return RedisInterface::execRedisCommandResult(socket, cmd);
}

RedisInterface::RedisData RedisInterface::execRedisCommandResult(QTcpSocket *socket, std::list<QByteArray> cmd)
{
    // send request
    if(!RedisInterface::execRedisCommand(socket, cmd)) return RedisData("Error by sending Request to Redis!");

    // return parsed response
    return RedisInterface::parseResponse(socket);
}

bool RedisInterface::execRedisCommand(RedisServer& server, std::list<QByteArray> cmd, bool writeOnly)
{
    // acquire socket
    QTcpSocket* socket = server.requestConnection(writeOnly ? RedisServer::ConnectionType::WriteOnly : RedisServer::ConnectionType::ReadWrite);

    return RedisInterface::execRedisCommand(socket, cmd);
}

bool RedisInterface::execRedisCommand(QTcpSocket *socket, std::list<QByteArray> cmd)
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

    // build packet
    *content++ = '*';
    itoa(cmd.size(), content, 10);
    content += (int)floor(log10(abs(cmd.size()))) + 1;
    content = (char*)mempcpy(content, "\r\n", 2);
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) {
        *content++ = '$';
        itoa(itr->isNull() ? -1 : itr->length(), content, 10);
        content += (int)itr->isNull() ? 2 : itr->isEmpty() ? 1 : (int)floor(log10(abs(itr->length()))) + 1;
        content = (char*)mempcpy(content, "\r\n", 2);
        if(!itr->isNull()) {
            content = (char*)mempcpy(content, itr->data(), itr->length());
            content = (char*)mempcpy(content, "\r\n", 2);
        }
    }

    // 3. write RESP request to socket
    socket->write(QByteArray::fromRawData(contentOriginPos, content - contentOriginPos));
    free(contentOriginPos);

    // everything okay
    return true;
}

RedisInterface::RedisData RedisInterface::parseResponse(QTcpSocket* socket)
{
    // check socket
    if(!socket || !socket->isReadable()) return RedisData("Socket not readable!");

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
    RedisData redisData(respDataType == '+' ? RedisData::Type::SimpleString :
                   respDataType == '-' ? RedisData::Type::Error :
                   respDataType == ':' ? RedisData::Type::Integer :
                   respDataType == '$' ? RedisData::Type::BulkString :
                   respDataType == '*' ? RedisData::Type::Array : RedisData::Type::Unknown);

    // handle base types (SimpleString, Error and Integer)
    if(redisData.type == RedisData::Type::SimpleString || redisData.type == RedisData::Type::Error || redisData.type == RedisData::Type::Integer) {
        // get pointer to end of current segment and be sure we have enough data available to read
        char* protoSegmentNext = readSegement(&rawData);

        // read segment (but without the segment end chars \r and \n)
        QByteArray segment = QByteArray(rawData, protoSegmentNext - rawData - 2);
        rawData = protoSegmentNext;

        // set data to redis structs
        if(redisData.type == RedisData::Type::SimpleString) redisData.string = segment;
        if(redisData.type == RedisData::Type::Error)        redisData.errorString = QString(segment);
        if(redisData.type == RedisData::Type::Integer)      redisData.integer = segment.toInt();
    }

    // handle BulkString
    else if(redisData.type == RedisData::Type::BulkString) {
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
        redisData.string = segmentData;
    }

    // handle Arrays and Multi Bulk Arrays
    else if(redisData.type == RedisData::Type::Array) {
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

                currentArray = &*redisData.arrayList.insert(--redisData.arrayList.begin(), std::list<QByteArray>());

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
                currentArray = &*--redisData.arrayList.end();
                currentElementCount = allElementsCount;
            }
        } while(--currentElementCount && --allElementsCount);

        // if we have less or equal one item, return a RedisArray otherwise a RedisArrayList
        if(redisData.arrayList.size() > 1) redisData.type = RedisData::Type::ArrayList;
        else if(redisData.arrayList.size() == 1) {
            redisData.array = *redisData.arrayList.begin();
            redisData.arrayList.clear();
        }
    }

    // this poistion should never we reached, but if, we return a unknown RedisData Type
    return redisData;
}
