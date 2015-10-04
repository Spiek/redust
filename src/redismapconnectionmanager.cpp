#include "redismapconnectionmanager.h"

QMap<QString, RedisConnectionPool*> RedisConnectionManager::mapConPools;

RedisConnectionPool::RedisConnectionPool()
{

}


bool RedisConnectionPool::addRedisConnections(QString redisServer, qint16 redisPort, int blockingConnections, int writeOnlyConnections)
{
    // init connections
    for(int type = 0; type < 2; type++) {
        auto queueConnection = !type ? &this->queueBlockingConnectionsAvailable : &this->queueWriteOnlyConnections;
        for(int iConnection = 0; iConnection < (!type ? blockingConnections : writeOnlyConnections); iConnection++) {
            // create new connection
            RedisConnection* conRedis = new RedisConnection;
            conRedis->redisServer = redisServer;
            conRedis->redisPort = redisPort;
            conRedis->socket = new QTcpSocket;
            conRedis->conPool = this;

            // connect to redis
            conRedis->socket->connectToHost(conRedis->redisServer, conRedis->redisPort, type ? QIODevice::WriteOnly : QIODevice::ReadWrite);
            if(!conRedis->socket->waitForConnected(5000)) {
                qFatal("Cannot connect to Redis Server %s:%i, give up...", qPrintable(conRedis->redisServer), conRedis->redisPort);
                delete conRedis;
                return false;
            }

            // save connection
            queueConnection->enqueue(conRedis);
        }
    }
    return true;
}

RedisConnection* RedisConnectionPool::requestConnection(bool writeOnly)
{
    RedisConnection* con = 0;
    if(writeOnly && !queueWriteOnlyConnections.isEmpty()) {
        con = this->queueWriteOnlyConnections.dequeue();
        this->queueWriteOnlyConnections.enqueue(con);
    }
    else if(!writeOnly && !queueBlockingConnectionsAvailable.isEmpty()) {
        con = this->queueBlockingConnectionsAvailable.dequeue();
        this->queueBlockingConnectionsBlocked.append(con);
    }
    return con;
}

void RedisConnectionPool::freeConnection(RedisConnection* connection)
{
    if(this->queueBlockingConnectionsBlocked.contains(connection)) {
        this->queueBlockingConnectionsBlocked.removeOne(connection);
        this->queueBlockingConnectionsAvailable.enqueue(connection);
    }
}
