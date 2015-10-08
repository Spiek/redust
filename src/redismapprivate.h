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
        void clear(bool async = true);
        bool insert(QByteArray key, QByteArray value, bool waitForAnswer = false);
        QByteArray value(QByteArray key);

    private:
        bool execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result = 0);

    private:
        QString connectionName;
        QByteArray redisList;
};

#endif // REDISMAPPRIVATE_H
