#ifndef REDISMAPCONNECTIONMANAGER_H
#define REDISMAPCONNECTIONMANAGER_H

#include <QMap>
#include <QThread>
#include <QMutex>
#include <QTcpSocket>

class RedisServer
{
    public:
        enum class ConnectionType {
            WriteOnly,
            ReadWrite,
            Blocked
        };
        RedisServer(QString redisServer, qint16 redisPort);
        QTcpSocket* requestConnection(RedisServer::ConnectionType type);
        void freeBlockedConnection(QTcpSocket *socket);

    private:
        QTcpSocket* constructNewConnection(bool writeOnly);

        // mutex
        static QMutex mutex;

        // connection queues
        QMap<QThread*, QTcpSocket*> mapConnectionsWriteOnly;
        QMap<QThread*, QTcpSocket*> mapConnectionsReadWrite;
        QMap<QThread*, QList<QTcpSocket*>> mapConnectionsBlocked;

        // redis connection data
        QString strRedisConnectionHost;
        quint16 intRedisConnectionPort;
};

#endif // REDISMAPCONNECTIONMANAGER_H
