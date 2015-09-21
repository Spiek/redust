#ifndef REDISMAPPRIVATE_H
#define REDISMAPPRIVATE_H

// qt core
#include <QString>
#include <QByteArray>
#include <QMutex>

// redis
#include "redismapconnectionmanager.h"

class RedisMapPrivate
{
    public:
        RedisMapPrivate(QString list, QString connectionName = "redis");
        bool insert(QByteArray key, QByteArray value, bool waitForAnswer = false);

    private:
        static inline bool checkRedisReturnValue(QAbstractSocket* socket);
        static void execRedisCommand(QAbstractSocket* socket, const char* cmd, ...);

    private:
        RedisMapConnectionManager::RedisConnection *redisConnection;
        QByteArray redisList;
};

#endif // REDISMAPPRIVATE_H
