#ifndef REDISMAPCONNECTIONMANAGER_H
#define REDISMAPCONNECTIONMANAGER_H

#include <QMap>
#include <QTcpSocket>

class RedisMapConnectionManager
{
    public:
        struct RedisConnection
        {
            QString redisServer;
            qint16 redisPort;
            QString connectionName;
            QTcpSocket *socket = 0;
        };

        static bool initRedisConnection(QString redisServer, qint16 redisPort, QString connectionName = "redis");
        static RedisConnection* getReadyRedisConnection(QString connectionName = "redis");
        static bool checkRedisConnection(RedisConnection *redisConnection);

    private:
        static QMap<QString, RedisMapConnectionManager::RedisConnection*> mapRedisConnections;
};

#endif // REDISMAPCONNECTIONMANAGER_H
