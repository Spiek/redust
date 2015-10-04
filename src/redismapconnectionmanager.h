#ifndef REDISMAPCONNECTIONMANAGER_H
#define REDISMAPCONNECTIONMANAGER_H

#include <QQueue>
#include <QMap>
#include <QTcpSocket>

// forward declarations
class RedisConnectionPool;
class RedisConnectionReleaser;

struct RedisConnection
{
    public:
        ~RedisConnection()
        {
            if(this->socket) this->socket->deleteLater();
        }
        QString redisServer;
        qint16 redisPort;
        QTcpSocket *socket = 0;

    private:
        RedisConnectionPool* conPool;

    friend class RedisConnectionReleaser;
    friend class RedisConnectionPool;
};

class RedisConnectionPool
{
    public:
        RedisConnectionPool();
        bool addRedisConnections(QString redisServer, qint16 redisPort, int blockingConnections = 2, int writeOnlyConnections = 1);
        RedisConnection* requestConnection(bool writeOnly);
        void freeConnection(RedisConnection *connection);

    private:
        // connection queues
        QQueue<RedisConnection*> queueBlockingConnectionsAvailable;
        QList<RedisConnection*> queueBlockingConnectionsBlocked;
        QQueue<RedisConnection*> queueWriteOnlyConnections;
};

class RedisConnectionReleaser
{
    public:
        RedisConnectionReleaser(RedisConnection* connection)
        {
            this->d = connection;
        }
        void free()
        {
            if(this->d) {
                this->d->conPool->freeConnection(this->d);
                this->d = 0;
            }
        }
        RedisConnection* data()
        {
            return this->d;
        }

        RedisConnection* operator->() {
            return this->d;
        }

        ~RedisConnectionReleaser()
        {
            this->free();
        }

        private:
            RedisConnection* d;
};

class RedisConnectionManager
{
    public:
        static bool addConnections(QString connectionName, QString redisServer, qint16 redisPort, int blockingConnections = 2, int writeOnlyConnections = 1)
        {
            // get pool instance
            RedisConnectionPool* pool = RedisConnectionManager::mapConPools.value(connectionName, 0);
            if(!pool) pool = RedisConnectionManager::mapConPools.insert(connectionName, new RedisConnectionPool).value();

            // insert connection into pool
            return pool->addRedisConnections(redisServer, redisPort, blockingConnections, writeOnlyConnections);
        }
        static RedisConnectionReleaser requestConnection(QString connectionName, bool writeOnly)
        {
            // get pool instance and try to request a connection
            RedisConnectionPool* pool = RedisConnectionManager::mapConPools.value(connectionName, 0);
            return RedisConnectionReleaser(pool ? pool->requestConnection(writeOnly) : 0);
        }

    private:
        static QMap<QString, RedisConnectionPool*> mapConPools;
};

#endif // REDISMAPCONNECTIONMANAGER_H
