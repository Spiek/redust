#ifndef REDISMAP_H
#define REDISMAP_H

#include <QMap>
#include <QVariant>
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

        static bool initRedisConnection(QString redisServer, qint16 redisPort, QString connectionName = "redis")
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

        static RedisConnection* getReadyRedisConnection(QString connectionName = "redis")
        {
            RedisConnection* rconnection = RedisMapConnectionManager::mapRedisConnections.value(connectionName);
            return rconnection && rconnection->socket->state() == QAbstractSocket::ConnectedState ? rconnection : 0;
        }

    private:
        static QMap<QString, RedisMapConnectionManager::RedisConnection*> mapRedisConnections;
};

template< class Key, class T >
class RedisMap
{
    public:
        RedisMap(QString list, QString connectionName = "redis")
        {
            this->redisList = list;
            this->redisCmd = QString("%2 %1.%3 ").arg(this->redisList);
            this->redisConnection = RedisMapConnectionManager::getReadyRedisConnection(connectionName);
        }

        bool insert(Key key, T value, bool waitForAnswer = false)
        {
            // if connection is not okay, exit
            if(!this->redisConnection || this->redisConnection->socket->state() != QAbstractSocket::ConnectedState) return false;

            // simplefy key and value
            QVariant vkey(key);
            QVariant vValue(value);

            // if something is wrong with key and value, exit
            if(!vkey.isValid() || !vValue.isValid()) return false;

            // insert into redis
            this->redisConnection->socket->write(this->redisCmd.arg("SET").arg(vkey.toString()).toLocal8Bit());
            this->redisConnection->socket->write(vValue.toByteArray());
            this->redisConnection->socket->write("\r\n");
            this->redisConnection->socket->flush();

            // if user want to wait until redis server answers, so do it
            if(waitForAnswer) {
                return this->redisConnection->socket->waitForBytesWritten() && this->redisConnection->socket->waitForReadyRead();
            }

            // otherwise everything is okay
            return true;
        }

    private:
        RedisMapConnectionManager::RedisConnection *redisConnection;
        QString redisList;
        QString redisCmd;
};

#endif // REDISMAP_H
