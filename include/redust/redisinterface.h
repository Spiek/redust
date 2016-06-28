#ifndef REDISMAPPRIVATE_H
#define REDISMAPPRIVATE_H

// qt core
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <list>

// redis
#include "redust/redisserver.h"

class RedisInterface
{
    public:
        struct RedisData
        {
            enum class Type {
                Unknown = 0,
                SimpleString = 1,
                Error = 2,
                Integer = 3,

                BulkString = 4,
                Array = 5,
                ArrayList = 6
            } type;
            QByteArray string;
            QString errorString;
            int integer;
            std::list<QByteArray> array;
            std::list<std::list<QByteArray>> arrayList;
            inline RedisData(RedisData::Type type) : type(type) { }
            inline RedisData(QString errorString) : type(RedisData::Type::Error), errorString(errorString) { }
        };

        enum class Position {
            Begin,
            End
        };

        // General Redis Functions
        static bool ping(RedisServer& server, QByteArray data = "", bool async = false);

        // Key-Value Redis Functions
        static bool del(RedisServer& server, QByteArray key, bool async = true);
        static bool exists(RedisServer& server, QByteArray key);
        static std::list<QByteArray> keys(RedisServer& server, QByteArray pattern = "*");

        // List Redis Functions
        static int push(RedisServer& server, QByteArray key, QByteArray value, RedisInterface::Position direction = Position::Begin, bool waitForAnswer = false);
        static int push(RedisServer& server, QByteArray key, std::list<QByteArray> values, RedisInterface::Position direction = Position::Begin, bool waitForAnswer = false);
        static bool bpop(QTcpSocket *socket, std::list<QByteArray> lists, RedisInterface::Position direction = Position::Begin, int timeout = 0);
        static int llen(RedisServer& server, QByteArray key);

        // Hash Redis Functions
        static int hlen(RedisServer& server, QByteArray list);
        static bool hset(RedisServer& server, QByteArray list, QByteArray key, QByteArray value, bool waitForAnswer = false, bool onlySetIfNotExists = false);
        static bool hmset(RedisServer& server, QByteArray list, std::list<QByteArray> keys, std::list<QByteArray> values, bool waitForAnswer = false);
        static bool hexists(RedisServer& server, QByteArray list, QByteArray key);
        static bool hdel(RedisServer& server, QByteArray list, QByteArray key, bool waitForAnswer = true);
        static QByteArray hget(RedisServer& server, QByteArray list, QByteArray key);
        static std::list<QByteArray> hmget(RedisServer& server, QByteArray list, std::list<QByteArray> keys);
        static int hstrlen(RedisServer& server, QByteArray list, QByteArray key);
        static void hkeys(RedisServer& server, QByteArray list, std::list<QByteArray>& result);
        static void hvals(RedisServer& server, QByteArray list, std::list<QByteArray>& result);
        static void hgetall(RedisServer& server, QByteArray list, std::list<QByteArray>& result);
        static void hgetall(RedisServer& server, QByteArray list, QMap<QByteArray, QByteArray>& keyValues);
        static void hgetall(RedisServer& server, QByteArray list, QHash<QByteArray, QByteArray>& keyValues);

        // Scan Redis Functions
        static void scan(RedisServer& server, QByteArray list, std::list<QByteArray>* keys, std::list<QByteArray>* values, int count = 100, int pos = 0, int *newPos = 0);
        static void scan(RedisServer& server, QByteArray list, std::list<QByteArray>& keyValues, int count = 100, int pos = 0, int *newPos = 0);
        static void scan(RedisServer& server, QByteArray list, QMap<QByteArray, QByteArray> &keyValues, int count = 100, int pos = 0, int *newPos = 0);

        // Redis Command Execution Helper
        static RedisData execRedisCommandResult(RedisServer& server, std::list<QByteArray> cmd);
        static RedisData execRedisCommandResult(QTcpSocket* socket, std::list<QByteArray> cmd);
        static bool execRedisCommand(RedisServer& server, std::list<QByteArray> cmd, bool writeOnly);
        static bool execRedisCommand(QTcpSocket* socket, std::list<QByteArray> cmd);
        static RedisData parseResponse(QTcpSocket* socket);

    private:
        // command simplifier
        static void simplifyHScan(RedisServer& server, QByteArray list, std::list<QByteArray> *lstKeyValues, std::list<QByteArray> *keys, std::list<QByteArray> *values, QMap<QByteArray, QByteArray> *keyValues, int count, int pos, int *newPos);

        // very fast implementation of integer places counting
        // src: http://stackoverflow.com/a/1068937
        static inline int numIntPlaces(int n) {
            if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
            if (n < 10) return 1;
            if (n < 100) return 2;
            if (n < 1000) return 3;
            if (n < 10000) return 4;
            if (n < 100000) return 5;
            if (n < 1000000) return 6;
            if (n < 10000000) return 7;
            if (n < 100000000) return 8;
            if (n < 1000000000) return 9;
            /*      2147483647 is 2^31-1 - add more ifs as needed
               and adjust this final return as well. */
            return 10;
        }
};

#endif // REDISMAPPRIVATE_H
