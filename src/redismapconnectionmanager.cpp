#include "redismapconnectionmanager.h"

QMap<QString, RedisMapConnectionManager::RedisConnection*> RedisMapConnectionManager::mapRedisConnections = QMap<QString, RedisMapConnectionManager::RedisConnection*>();

bool RedisMapConnectionManager::initRedisConnection(QString redisServer, qint16 redisPort, QString connectionName)
{
    // acquire redis connection
    RedisConnection* conRedis = RedisMapConnectionManager::mapRedisConnections.take(connectionName);
    if(!conRedis) conRedis = new RedisConnection;

    // set redis data
    conRedis->redisServer = redisServer;
    conRedis->redisPort = redisPort;
    conRedis->connectionName = connectionName;

    // try to connec to the redis server
    if(conRedis->socket) conRedis->socket->deleteLater();
    conRedis->socket = new QTcpSocket;
    conRedis->socket->connectToHost(redisServer, redisPort);
    if(!conRedis->socket->waitForConnected(5000)) {
        conRedis->socket->deleteLater();
        return false;
    }

    // on success connection, save the connection
    RedisMapConnectionManager::mapRedisConnections.insert(connectionName, conRedis);
    return true;
}

RedisMapConnectionManager::RedisConnection* RedisMapConnectionManager::getReadyRedisConnection(QString connectionName)
{
    RedisMapConnectionManager::RedisConnection* rconnection = RedisMapConnectionManager::mapRedisConnections.value(connectionName);
    return RedisMapConnectionManager::checkRedisConnection(rconnection) ? rconnection : 0;
}

bool RedisMapConnectionManager::checkRedisConnection(RedisMapConnectionManager::RedisConnection *redisConnection)
{
    return redisConnection && redisConnection->socket->state() == QAbstractSocket::ConnectedState;
}

