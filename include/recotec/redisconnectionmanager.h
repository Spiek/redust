#ifndef REDISMAPCONNECTIONMANAGER_H
#define REDISMAPCONNECTIONMANAGER_H

#include <QMap>
#include <QThread>
#include <QMutex>
#include <QTcpSocket>

class RedisConnectionPool
{
    public:
        RedisConnectionPool(QString redisServer, qint16 redisPort);
        QTcpSocket* requestConnection(bool writeOnly);

    private:
        // connection queues
        QMap<QThread*, QTcpSocket*> mapThreadRedisConnections;
        QMap<QThread*, QTcpSocket*> mapWriteOnlyThreadRedisConnections;
        QString strRedisConnectionHost;
        quint16 intRedisConnectionPort;
};

class RedisConnectionManager
{
    public:
        static bool addConnection(QString connectionName, QString redisServer, qint16 redisPort)
        {
            // lock context
            QMutexLocker mlocker(&RedisConnectionManager::mutex);

            // create pool instance if not allready happened
            if(!RedisConnectionManager::mapConPools.contains(connectionName)) {
                RedisConnectionManager::mapConPools.insert(connectionName, new RedisConnectionPool(redisServer, redisPort));
            }
            return true;
        }

        static QTcpSocket* requestConnection(QString connectionName, bool writeOnly)
        {
            // lock context
            QMutexLocker mlocker(&RedisConnectionManager::mutex);

            // get pool instance and try to request a connection
            RedisConnectionPool* pool = RedisConnectionManager::mapConPools.value(connectionName);
            return pool ? pool->requestConnection(writeOnly) : 0;
        }

    private:
        static QMutex mutex;
        static QMap<QString, RedisConnectionPool*> mapConPools;
};

#endif // REDISMAPCONNECTIONMANAGER_H
