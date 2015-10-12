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
        QList<QByteArray> keys(int count = 100, int pos = 0, int *newPos = 0);
        QList<QByteArray> values(int count = 100, int pos = 0, int *newPos = 0);

        // command simplifier
        QList<QByteArray> simplifyHScan(int count, int pos, bool key, bool value, int *newPos);

    private:
        // helper
        bool execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result = 0, QList<QByteArray> *lstResultArray1 = 0, QList<QByteArray> *lstResultArray2 = 0);

        // data
        QString connectionName;
        QByteArray redisList;
};

#endif // REDISMAPPRIVATE_H
