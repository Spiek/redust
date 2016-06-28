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
        enum PollTimeType
        {
            UntilTimeout = 1,
            UntilFirstPop = 2,
            Forever = 4
        };
        RedisListPoller(RedisServer &server, std::list<QByteArray> lstKeys, int timeout = 0, PollTimeType pollTimeType = PollTimeType::Forever, RedisInterface::Position popDirection = RedisInterface::Position::Begin, QObject *parent = 0);
        ~RedisListPoller();
        bool start();
        void stop(bool instantly = false);

        // check functions
        inline bool isRunning() { return !this->suspended && this->socket && this->socket->isReadable(); }

        // getter / setter
        inline std::list<QByteArray> keys() { return this->lstKeys; }
        inline void setKeys(std::list<QByteArray> keys) { this->lstKeys = keys; }
        inline int timeout() { return this->intTimeout; }
        inline void setTimeout(int timeout) { this->intTimeout = timeout; }
        inline PollTimeType pollTimeType() { return this->enumPollTimeType; }
        inline void setPollTimeType(PollTimeType pollTimeType) { this->enumPollTimeType = pollTimeType; }

    signals:
        void timeoutReached();
        void popped(QByteArray list, QByteArray value);

    private slots:
        bool pop();
        void handleResponse();
        bool acquireSocket();
        void releaseSocket();

    private:
        int intTimeout;
        bool suspended = false;
        PollTimeType enumPollTimeType;
        std::list<QByteArray> lstKeys;
        RedisInterface::Position popDirection;
        RedisServer* server = 0;
        QTcpSocket* socket = 0;
};

#endif // REDISLISTPOLLER_H
