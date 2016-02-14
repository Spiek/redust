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
        static bool ping(RedisServer& server, QByteArray data = "", bool async = false);

        // Key-Value Redis Functions
        static bool del(RedisServer& server, QByteArray key, bool async = true);
        static bool exists(RedisServer& server, QByteArray key);
        static QList<QByteArray> keys(RedisServer& server, QByteArray pattern = "*");

        // Hash Redis Functions
        static int hlen(RedisServer& server, QByteArray list);
        static bool hset(RedisServer& server, QByteArray list, QByteArray key, QByteArray value, bool waitForAnswer = false, bool onlySetIfNotExists = false);
        static bool hmset(RedisServer& server, QByteArray list, QList<QByteArray> keys, QList<QByteArray> values, bool waitForAnswer = false);
        static bool hexists(RedisServer& server, QByteArray list, QByteArray key);
        static bool hdel(RedisServer& server, QByteArray list, QByteArray key, bool waitForAnswer = true);
        static QByteArray hget(RedisServer& server, QByteArray list, QByteArray key);
        static QList<QByteArray> hmget(RedisServer& server, QByteArray list, QList<QByteArray> keys);
        static int hstrlen(RedisServer& server, QByteArray list, QByteArray key);
        static void hkeys(RedisServer& server, QByteArray list, QList<QByteArray>& result);
        static void hvals(RedisServer& server, QByteArray list, QList<QByteArray>& result);
        static void hgetall(RedisServer& server, QByteArray list, QList<QByteArray>& result);
        static void hgetall(RedisServer& server, QByteArray list, QMap<QByteArray, QByteArray>& keyValues);
        static void hgetall(RedisServer& server, QByteArray list, QHash<QByteArray, QByteArray>& keyValues);

        // Scan Redis Functions
        static void scan(RedisServer& server, QByteArray list, QList<QByteArray>* keys, QList<QByteArray>* values, int count = 100, int pos = 0, int *newPos = 0);
        static void scan(RedisServer& server, QByteArray list, QList<QByteArray>& keyValues, int count = 100, int pos = 0, int *newPos = 0);
        static void scan(RedisServer& server, QByteArray list, QMap<QByteArray, QByteArray> &keyValues, int count = 100, int pos = 0, int *newPos = 0);

    private:
        // command simplifier
        static void simplifyHScan(RedisServer& server, QByteArray list, QList<QByteArray> *lstKeyValues, QList<QByteArray> *keys, QList<QByteArray> *values, QMap<QByteArray, QByteArray> *keyValues, int count, int pos, int *newPos);

        // helper
        static bool execRedisCommand(RedisServer& server, QList<QByteArray> cmd, QByteArray* result = 0, QList<QByteArray> *resultArray = 0, QList<QList<QByteArray>>* result2dArray = 0, bool blockedConnection = false);
};

#endif // REDISMAPPRIVATE_H
