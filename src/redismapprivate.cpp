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
    QByteArray* baList = &this->redisList;
    static const char* redisCmd = "*3\r\n$3\r\nSET\r\n$%d\r\n%s%s\r\n$%d\r\n%s\r\n";
    RedisMapPrivate::execRedisCommand(this->redisConnection->socket,
                                // command
                                redisCmd,

                                // command params
                                key.size() + baList->size(),
                                baList->data(),
                                key.data(),
                                value.size(),
                                value.data());

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

void RedisMapPrivate::execRedisCommand(QAbstractSocket *socket, const char *cmd, ...)
{
    // check socket
    if(!socket) return;

    // init some static buffer vars
    static int sizeBuffer = REDISMAP_INIT_QUERY_BUFFER_CACHE_SIZE;
    static char* buffer = (char*)malloc(REDISMAP_INIT_QUERY_BUFFER_CACHE_SIZE);

    // only create one command at a time
    static QMutex mutex;
    QMutexLocker mlocker(&mutex);

    // create command in buffer (and resize buffer if needed!)
    va_list ap;
    va_start(ap, cmd);
    int len = vsnprintf(buffer, sizeBuffer, cmd, ap);
    if(len > sizeBuffer) {
        sizeBuffer = len;
        buffer = (char*)realloc(buffer, sizeBuffer);
        va_start(ap, cmd);
        len = vsnprintf(buffer, sizeBuffer, cmd, ap);
    }

    // write buffer to
    socket->write(buffer, len);

    // if we are reaching the max buffer cache, we decrease the size to max allowed
    if(sizeBuffer > REDISMAP_MAX_QUERY_BUFFER_CACHE_SIZE) {
        sizeBuffer = REDISMAP_MAX_QUERY_BUFFER_CACHE_SIZE;
        buffer = (char*)realloc(buffer, REDISMAP_MAX_QUERY_BUFFER_CACHE_SIZE);
    }
}
