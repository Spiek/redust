#ifndef REDISMAP_H
#define REDISMAP_H

#include <QVariant>
#include <QMutex>

// own
#include "redismapconnectionmanager.h"

// some static settings
#define INIT_QUERY_BUFFER_CACHE_SIZE 1
#define MAX_QUERY_BUFFER_CACHE_SIZE 10485760

template< typename Key, typename Value >
class RedisMap
{
    public:
        RedisMap(QString list, QString connectionName = "redis")
        {
            this->redisList = list.toLocal8Bit() + ".";
            this->redisConnection = RedisMapConnectionManager::getReadyRedisConnection(connectionName);
        }

        bool insert(Key key, Value value, bool waitForAnswer = false)
        {
            // if connection is not okay, exit
            if(!RedisMapConnectionManager::checkRedisConnection(this->redisConnection)) return false;

            // simplefy key and value
            QVariant vkey(key);
            QVariant vValue(value);

            // if something is wrong with key or value, exit
            if(!vkey.isValid() || !vValue.isValid()) return false;

            // simplify command params
            QByteArray baKey = vkey.toByteArray();
            QByteArray baValue = vValue.toByteArray();

            // Build and execute Command
            // SET key value
            // src: http://redis.io/commands/SET
            QByteArray* baList = &this->redisList;
            static const char* redisCmd = "*3\r\n$3\r\nSET\r\n$%d\r\n%s%s\r\n$%d\r\n%s\r\n";
            RedisMap::execRedisCommand(this->redisConnection->socket,
                                        // command
                                        redisCmd,

                                        // command params
                                        baKey.size() + baList->size(),
                                        baList->data(),
                                        baKey.data(),
                                        baValue.size(),
                                        baValue.data());

            // if user want to wait until redis server answers, so do it
            if(waitForAnswer) {
                this->redisConnection->socket->waitForBytesWritten();
                this->redisConnection->socket->waitForReadyRead();
                return RedisMap::checkRedisReturnValue(this->redisConnection->socket);
            }

            // otherwise everything is okay
            return true;
        }

        static inline bool checkRedisReturnValue(QAbstractSocket* socket)
        {
            return socket && socket->readAll().left(3) == "+OK";
        }

        static void execRedisCommand(QAbstractSocket* socket, const char* cmd, ...)
        {
            // check socket
            if(!socket) return;

            // build command
            static int sizeBuffer = INIT_QUERY_BUFFER_CACHE_SIZE;
            static char* buffer = (char*)malloc(INIT_QUERY_BUFFER_CACHE_SIZE);

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
            if(sizeBuffer > MAX_QUERY_BUFFER_CACHE_SIZE) {
                sizeBuffer = MAX_QUERY_BUFFER_CACHE_SIZE;
                buffer = (char*)realloc(buffer, MAX_QUERY_BUFFER_CACHE_SIZE);
            }
        }

    private:
        RedisMapConnectionManager::RedisConnection *redisConnection;
        QByteArray redisList;
};

#endif // REDISMAP_H
