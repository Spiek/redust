#ifndef REDISMAPPRIVATE_H
#define REDISMAPPRIVATE_H

// qt core
#include <QString>
#include <QByteArray>
#include <QMutex>

// redis
#include "redismapconnectionmanager.h"

class RedisInterface
{
    public:
        RedisInterface(QString list, QString connectionName = "redis");

        // Key-Value Redis Functions
        bool del(bool async = true);
        bool exists();

        // Hash Redis Functions
        int hlen();
        bool hset(QByteArray key, QByteArray value, bool waitForAnswer = false);
        bool hexists(QByteArray key);
        bool hdel(QByteArray key, bool waitForAnswer = true);
        QByteArray hget(QByteArray key);
        void hkeys(QList<QByteArray>& result);
        void hvals(QList<QByteArray>& result);
        void hgetall(QList<QByteArray>& result);
        void hgetall(QMap<QByteArray, QByteArray>& keyValues);
        void hgetall(QHash<QByteArray, QByteArray>& keyValues);

        // Scan Redis Functions
        void scan(QList<QByteArray>* keys, QList<QByteArray>* values, int count = 100, int pos = 0, int *newPos = 0);
        void scan(QList<QByteArray>& keyValues, int count = 100, int pos = 0, int *newPos = 0);
        void scan(QMap<QByteArray, QByteArray> &keyValues, int count = 100, int pos = 0, int *newPos = 0);

    private:
        // command simplifier
        void simplifyHScan(QList<QByteArray> *lstKeyValues, QList<QByteArray> *keys, QList<QByteArray> *values, QMap<QByteArray, QByteArray> *keyValues, int count, int pos, int *newPos);

        // helper
        bool execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result = 0, QList<QByteArray> *lstResultArray1 = 0, QList<QByteArray> *lstResultArray2 = 0);

        // data
        QString connectionName;
        QByteArray redisList;
};

#endif // REDISMAPPRIVATE_H
