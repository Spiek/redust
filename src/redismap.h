#ifndef REDISMAP_H
#define REDISMAP_H

#include <QVariant>

// own
#include "redismapconnectionmanager.h"

template< class Key, class T >
class RedisMap
{
    public:
        RedisMap(QString list, QString connectionName = "redis")
        {
            this->redisList = list.toLocal8Bit() + ".";
            this->redisConnection = RedisMapConnectionManager::getReadyRedisConnection(connectionName);
        }

        bool insert(Key key, T value, bool waitForAnswer = false)
        {
            // if connection is not okay, exit
            if(!RedisMapConnectionManager::checkRedisConnection(this->redisConnection)) return false;

            // simplefy key and value
            QVariant vkey(key);
            QVariant vValue(value);

            // if something is wrong with key or value, exit
            if(!vkey.isValid() || !vValue.isValid()) return false;

            // build command
            QByteArray baKey = vkey.toByteArray().prepend(this->redisList);
            QByteArray baValue = vValue.toByteArray();

            QByteArray cmd = this->buildRedisCommand("*3\r\n$3\r\nSET\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n",
                                                        baKey.size(),
                                                        baKey.data(),
                                                        baValue.size(),
                                                        baValue.data()
                                                      );
            // insert into redis
            this->redisConnection->socket->write(cmd);

            // if user want to wait until redis server answers, so do it
            if(waitForAnswer) {
                this->redisConnection->socket->waitForBytesWritten();
                this->redisConnection->socket->waitForReadyRead();
            }

            // otherwise everything is okay
            return true;
        }

        QByteArray buildRedisCommand(const char* cmd, ...)
        {
            QByteArray buffer(1024, '\0');
            va_list ap;
            va_start(ap, cmd);
            int len = vsnprintf(buffer.data(), buffer.size(), cmd, ap);
            if(len >= buffer.size()) {
                buffer.resize(len + 1);

                va_start(ap, cmd);
                len = vsnprintf(buffer.data(), buffer.size(), cmd, ap);
            } else buffer.resize(len);

            return buffer;
        }


    private:
        RedisMapConnectionManager::RedisConnection *redisConnection;
        QByteArray redisList;
};

#endif // REDISMAP_H
