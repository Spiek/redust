#include "redismapprivate.h"

RedisMapPrivate::RedisMapPrivate(QString list, QString connectionName)
{
    this->redisList = list.toLocal8Bit() + ".";
    this->connectionName = connectionName;
}

bool RedisMapPrivate::insert(QByteArray key, QByteArray value, bool waitForAnswer)
{
    // Build and execute Command
    // SET key value
    // src: http://redis.io/commands/SET
    QByteArray returnValue;
    bool result = RedisMapPrivate::execRedisCommand({ "SET", key.prepend(this->redisList), value }, waitForAnswer ? &returnValue : 0);

    // determinate result
    if(!waitForAnswer) return result;
    else return result && returnValue == "OK";
}

QByteArray RedisMapPrivate::value(QByteArray key)
{
    // Build and execute Command
    // GET key
    // src: http://redis.io/commands/GET
    QByteArray returnValue;
    RedisMapPrivate::execRedisCommand({ "GET", key.prepend(this->redisList) }, &returnValue);

    // return result
    return returnValue;
}

bool RedisMapPrivate::execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result)
{
    // acquire socket
    RedisConnectionReleaser con = RedisConnectionManager::requestConnection(this->connectionName, !result);
    if(!con.data() || !con->socket) return false;

    /// Build RESP Array
    /// Note: Max 9.999.999.999 parameters/elements allowed -> ceil(log10(pow(256, sizeof(int)))
    /// see: http://redis.io/topics/protocol#resp-arrays
    // 1. build RESP number of elements size
    QByteArray content("*");
    char contentLength[11];
    content.append(itoa(cmd.size(), contentLength, 10));
    content.append("\r\n");

    // 2. build RESP elements
    for(auto itr = cmd.begin(); itr != cmd.end(); itr++) {
        content.append("$");
        content.append(itr->isNull() ? "-1" : itoa(itr->length(), contentLength, 10));
        content.append("\r\n");
        content.append(*itr);
        content.append("\r\n");
    }

    // write content to socket
    con->socket->write(content);

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
