#include "redismapconnectionmanager.h"

QMap<QString, RedisConnectionPool*> RedisConnectionManager::mapConPools;
QMutex RedisConnectionManager::mutex;

RedisConnectionPool::RedisConnectionPool(QString redisServer, qint16 redisPort)
{
    this->strRedisConnectionHost = redisServer;
    this->intRedisConnectionPort = redisPort;
}

QTcpSocket* RedisConnectionPool::requestConnection(bool writeOnly)
{
    // try to acquire existing redis connection for thread
    QThread* currentThread = QThread::currentThread();
    QTcpSocket* con = writeOnly ? this->mapWriteOnlyThreadRedisConnections.value(currentThread) : this->mapThreadRedisConnections.value(currentThread);

    // if not available create a new one
    if(!con) {
        // create new connection
        QTcpSocket* socket = new QTcpSocket;
        socket->connectToHost(this->strRedisConnectionHost, this->intRedisConnectionPort, writeOnly ? QIODevice::WriteOnly : QIODevice::ReadWrite);
        if(!socket->waitForConnected(5000)) {
            qFatal("Cannot connect to Redis Server %s:%i, give up...", qPrintable(this->strRedisConnectionHost), this->intRedisConnectionPort);
            delete socket;
            return 0;
        }

        // and save it into the map
        con = (writeOnly ? &this->mapWriteOnlyThreadRedisConnections : &this->mapThreadRedisConnections)->insert(currentThread, socket).value();
    }

    // return the acquired connection
    return con;
}
