#include "redismapprivate.h"

RedisMapPrivate::RedisMapPrivate(QString list, QString connectionName)
{
    this->redisList = list.toLocal8Bit() + ".";
    this->redisConnection = RedisMapConnectionManager::getReadyRedisConnection(connectionName);
}

bool RedisMapPrivate::insert(QByteArray key, QByteArray value, bool waitForAnswer)
{
    // if connection is not okay, exit
    if(!RedisMapConnectionManager::checkRedisConnection(this->redisConnection)) return false;

    // Build and execute Command
    // SET key value
    // src: http://redis.io/commands/SET
    RedisMapPrivate::execRedisCommand(this->redisConnection->socket,
                                      { "SET", key.prepend(this->redisList), value });

    // if user want to wait until redis server answers, so do it
    if(waitForAnswer) {
        this->redisConnection->socket->waitForBytesWritten();
        this->redisConnection->socket->waitForReadyRead();
        return RedisMapPrivate::checkRedisReturnValue(this->redisConnection->socket);
    }

    // otherwise everything is okay
    return true;
}

bool RedisMapPrivate::checkRedisReturnValue(QAbstractSocket *socket)
{
    return socket && socket->readAll().left(3) == "+OK";
}

void RedisMapPrivate::execRedisCommand(QAbstractSocket *socket, std::initializer_list<QByteArray> cmd)
{
    // check socket
    if(!socket) return;

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
    socket->write(content);
}
