#include "recotec/redisconnectionmanager.h"

QMutex RedisServer::mutex;

RedisServer::RedisServer(QString redisServer, qint16 redisPort)
{
    this->strRedisConnectionHost = redisServer;
    this->intRedisConnectionPort = redisPort;
}

QTcpSocket* RedisServer::requestConnection(RedisServer::ConnectionType type)
{
    // lock mutex
    QMutexLocker locker(&this->mutex);

    // try to acquire existing redis connection for thread
    QThread* currentThread = QThread::currentThread();

    // try to acquire a connection for read/write or write only access
    if(type == ConnectionType::ReadWrite || type == ConnectionType::WriteOnly) {
        QMap<QThread*, QTcpSocket*>& mapConList = type == ConnectionType::WriteOnly ? this->mapConnectionsWriteOnly : this->mapConnectionsReadWrite;
        QTcpSocket* con = mapConList.value(currentThread, 0);
        if(!con) {
            con = this->constructNewConnection(type == ConnectionType::WriteOnly);
            if(con) mapConList.insert(currentThread, con);
        }
        return con;
    }

    // acqure blocked list for thread
    QList<QTcpSocket*>& mapBlockedList = this->mapConnectionsBlocked.contains(currentThread) ?
                                            this->mapConnectionsBlocked.find(currentThread).value() :
                                            this->mapConnectionsBlocked.insert(currentThread, QList<QTcpSocket*>()).value();

    // if we dont have a connection left, create a new socket, otherwise take one and return it
    if(mapBlockedList.isEmpty()) return this->constructNewConnection(false);
    else return mapBlockedList.takeFirst();
}

void RedisServer::freeBlockedConnection(QTcpSocket *socket)
{
    // lock mutex
    QMutexLocker locker(&this->mutex);

    // try to acquire existing redis connection for thread
    QThread* currentThread = QThread::currentThread();

    // acqure blocked list for thread
    QList<QTcpSocket*>& mapBlockedList = this->mapConnectionsBlocked.contains(currentThread) ?
                                            this->mapConnectionsBlocked.find(currentThread).value() :
                                            this->mapConnectionsBlocked.insert(currentThread, QList<QTcpSocket*>()).value();
    // append socket
    mapBlockedList.append(socket);
}

QTcpSocket* RedisServer::constructNewConnection(bool writeOnly)
{
    // create new connection
    QTcpSocket* socket = new QTcpSocket;
    socket->connectToHost(this->strRedisConnectionHost, this->intRedisConnectionPort, writeOnly ? QIODevice::WriteOnly : QIODevice::ReadWrite);
    if(!socket->waitForConnected(5000)) {
        qFatal("Cannot connect to Redis Server %s:%i, give up...", qPrintable(this->strRedisConnectionHost), this->intRedisConnectionPort);
        delete socket;
        return 0;
    }
    return socket;
}
