#ifndef REDISMAPPRIVATE_H
#define REDISMAPPRIVATE_H

// qt core
#include <QString>
#include <QByteArray>
#include <QMutex>

// redis
#include "recotec/redisconnectionmanager.h"

class RedisInterface
{
    public:
        // General Redis Functions
        static bool ping(QByteArray data = "", bool async = false, QString connectionPool = "redis");

        // Key-Value Redis Functions
        static bool del(QByteArray key, bool async = true, QString connectionPool = "redis");
        static bool exists(QByteArray key, QString connectionPool = "redis");
        static QList<QByteArray> keys(QByteArray pattern = "*", QString connectionPool = "redis");

        // Hash Redis Functions
        static int hlen(QByteArray list, QString connectionPool = "redis");
        static bool hset(QByteArray list, QByteArray key, QByteArray value, bool waitForAnswer = false, QString connectionPool = "redis");
        static bool hmset(QByteArray list, QList<QByteArray> keys, QList<QByteArray> values, bool waitForAnswer = false, QString connectionPool = "redis");
        static bool hexists(QByteArray list, QByteArray key, QString connectionPool = "redis");
        static bool hdel(QByteArray list, QByteArray key, bool waitForAnswer = true, QString connectionPool = "redis");
        static QByteArray hget(QByteArray list, QByteArray key, QString connectionPool = "redis");
        static QList<QByteArray> hmget(QByteArray list, QList<QByteArray> keys, QString connectionPool = "redis");
        static void hkeys(QByteArray list, QList<QByteArray>& result, QString connectionPool = "redis");
        static void hvals(QByteArray list, QList<QByteArray>& result, QString connectionPool = "redis");
        static void hgetall(QByteArray list, QList<QByteArray>& result, QString connectionPool = "redis");
        static void hgetall(QByteArray list, QMap<QByteArray, QByteArray>& keyValues, QString connectionPool = "redis");
        static void hgetall(QByteArray list, QHash<QByteArray, QByteArray>& keyValues, QString connectionPool = "redis");

        // Scan Redis Functions
        static void scan(QByteArray list, QList<QByteArray>* keys, QList<QByteArray>* values, int count = 100, int pos = 0, int *newPos = 0, QString connectionPool = "redis");
        static void scan(QByteArray list, QList<QByteArray>& keyValues, int count = 100, int pos = 0, int *newPos = 0, QString connectionPool = "redis");
        static void scan(QByteArray list, QMap<QByteArray, QByteArray> &keyValues, int count = 100, int pos = 0, int *newPos = 0, QString connectionPool = "redis");

    private:
        // command simplifier
        static void simplifyHScan(QByteArray list, QList<QByteArray> *lstKeyValues, QList<QByteArray> *keys, QList<QByteArray> *values, QMap<QByteArray, QByteArray> *keyValues, int count, int pos, int *newPos, QString connectionPool = "redis");

        // helper
        static bool execRedisCommand(QList<QByteArray> cmd, QString connectionPool = "redis", QByteArray* result = 0, QList<QByteArray> *lstResultArray1 = 0, QList<QByteArray> *lstResultArray2 = 0);
};

#endif // REDISMAPPRIVATE_H
