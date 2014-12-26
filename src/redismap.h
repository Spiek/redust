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
            return RedisMapConnectionManager::checkRedisConnection(rconnection) ? rconnection : 0;
        }

        static bool checkRedisConnection(RedisConnection *redisConnection)
        {
            return redisConnection && redisConnection->socket->state() == QAbstractSocket::ConnectedState;
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
            this->redisList = list.toLocal8Bit() + ".";
            this->redisConnection = RedisMapConnectionManager::getReadyRedisConnection(connectionName);
        }

        bool insert(Key key, T value, bool waitForAnswer = false)
        {
            // if connection is not okay, exit
            if(!RedisMapConnectionManager::checkRedisConnection(this->redisConnection)) return false;

            // simplefy key and value
            QVariant vkey(key);
            QVariant vValue(value);

            // if something is wrong with key and value, exit
            if(!vkey.isValid() || !vValue.isValid()) return false;

            // build command
            QByteArray cmd = this->buildRedisCommand({
                                    "SET",
                                    this->redisList.append(vkey.toByteArray()),
                                    vValue.toByteArray()
                                });
            // insert into redis
            this->redisConnection->socket->write(cmd);

            // if user want to wait until redis server answers, so do it
            if(waitForAnswer) {
                this->redisConnection->socket->waitForBytesWritten();
                this->redisConnection->socket->waitForReadyRead();
            }

            // otherwise everything is okay
            return true;
        }

        QByteArray buildRedisCommand(std::initializer_list<QByteArray> args)
        {
            QByteArray data;
            char length[sizeof(int)*8+1+3];
            sprintf(length, "*%d\r\n", args.size());
            data += length;
            for(QByteArray arg : args) {
                sprintf(length, "$%d\r\n", arg.size());
                data += length;
                data += arg + "\r\n";
            }
            return data;
        }


    private:
        RedisMapConnectionManager::RedisConnection *redisConnection;
        QByteArray redisList;
};

#endif // REDISMAP_H
