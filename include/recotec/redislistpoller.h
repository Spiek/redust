#ifndef REDISLISTPOLLER_H
#define REDISLISTPOLLER_H

#include <QObject>

// redis
#include "typeserializer.h"
#include "redisinterface.h"

// recotec
#include "recotec/redisconnectionmanager.h"

class RedisListPoller : public QObject
{
    Q_OBJECT
    public:
        RedisListPoller(RedisServer &server, std::list<QByteArray> lstKeys, int timeout = 0, RedisInterface::Position popDirection = RedisInterface::Position::Begin, QObject *parent = 0);
        ~RedisListPoller();
        bool start();
        void stop(bool instantly = false);

        // check functions
        inline bool isRunning() { return this->socket && this->socket->isReadable(); }

        // getter / setter
        inline std::list<QByteArray> getKeys() { return this->lstKeys; }
        inline void setKeys(std::list<QByteArray> keys) { this->lstKeys = keys; }
        inline int getTimeout() { return this->intTimeout; }
        inline void setTimeout(int timeout) { this->intTimeout = timeout; }

    signals:
        void timeout();
        void popped(QByteArray list, QByteArray value);

    private slots:
        void handleResponse();

    private:
        int intTimeout;
        bool suspended = false;
        std::list<QByteArray> lstKeys;
        RedisInterface::Position popDirection;
        RedisServer* server = 0;
        QTcpSocket* socket = 0;
};

#endif // REDISLISTPOLLER_H
