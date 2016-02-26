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
        RedisListPoller(RedisServer &server, QList<QByteArray> lstKeys, RedisInterface::PopDirection popDirection = RedisInterface::PopDirection::Begin, int timeout = 0, QObject *parent = 0);
        ~RedisListPoller();
        bool start();
        void stop(bool instantly = false);

        // check functions
        inline bool isRunning() { return this->socket && this->socket->isReadable(); }

        // getter / setter
        inline QList<QByteArray> getKeys() { return this->lstKeys; }
        inline void setKeys(QList<QByteArray> keys) { this->lstKeys = keys; }
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
        QList<QByteArray> lstKeys;
        RedisInterface::PopDirection popDirection;
        RedisServer* server = 0;
        QTcpSocket* socket = 0;
};

#endif // REDISLISTPOLLER_H
