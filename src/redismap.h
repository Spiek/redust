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
            QByteArray cmd = this->buildRedisCommand({
                                    "SET",
                                    this->redisList.append(vkey.toByteArray()),
                                    vValue.toByteArray()
                                });

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

        QByteArray buildRedisCommand(std::initializer_list<QByteArray> args)
        {
            // Build binary save redis command as ByteArray in the following form:
            // *<Number of Arguments>\r\n
            // For Every argument:
            // $<number of bytes of argument>\r\n
            // <argument data>\r\n
            //
            QByteArray data;
            char length[sizeof(int)*8+1+3];
            sprintf(length, "*%d\r\n", args.size());
            data += length;
            for(QByteArray arg : args) {
                sprintf(length, "$%d\r\n", arg.size());
                data += length;
                data += arg + "\r\n";
            }
            return data;
        }


    private:
        RedisMapConnectionManager::RedisConnection *redisConnection;
        QByteArray redisList;
};

#endif // REDISMAP_H
