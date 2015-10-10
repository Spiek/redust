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

bool RedisMapPrivate::execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result)
{
    // acquire socket
    RedisConnectionReleaser con = RedisConnectionManager::requestConnection(this->connectionName, !result);
    if(!con.data() || !con->socket) return false;

    /// Build RESP Data Packet
    /// see: http://redis.io/topics/protocol#resp-arrays
    // 1. determine allocation size

    int size = 15;
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) size += 15 + itr->length();

    // 2. build RESP Data Packet
    char* content = (char*)malloc(size);
    char* stringData = content;
    content += sprintf(content, "*%i\r\n", cmd.size());
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) {
        content += sprintf(content, "$%i\r\n", itr->isEmpty() ? -1 : itr->length());
        content = (char*)mempcpy(content, itr->data(), itr->length());
        *content = '\r';
        *++content = '\n';
        content++;
    }

    // write content to socket
    con->socket->write(stringData, content - stringData);
    free(stringData);

    // exit if we don't have to parse the return code
    if(!result) return true;

    // wait for server return code
    con->socket->waitForReadyRead();
    QByteArray data = con->socket->readAll();

    /// Parse Result RESP Data
    /// see: http://redis.io/topics/protocol#resp-protocol-description

    char respDataType = data.at(0);
    data.remove(0, 1);

    // handle Simple Strings
    if(respDataType == '+') {
        *result = data.remove(data.length() - 2, 2);
        return true;
    }

    // handle Errors
    if(respDataType == '-') {
        *result = data.remove(data.length() - 2, 2);
        return false;
    }

    // handle Integers
    if(respDataType == ':') {
        *result = data.remove(data.length() - 2, 2);
        return true;
    }

    // handle Bulk Strings
    if(respDataType == '$') {
        char contentLength[11];
        char* nextChar = (char*)memccpy(contentLength, data.data(), '\r', 9);
        if(!nextChar) return false;
        *--nextChar = '\0';
        *result = data.mid(strlen(contentLength) + 2, atoi(contentLength));
        return true;
    }

    // otherwise we have an parse error
    return false;
}
