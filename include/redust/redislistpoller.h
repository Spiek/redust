#ifndef REDISLISTPOLLER_H
#define REDISLISTPOLLER_H

#include <QObject>

// redis
#include "typeserializer.h"

// redust
#include "redust/redisserver.h"

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
        enum PopPosition
        {
            Begin,
            End
        };

        // con/deconstructors
        RedisListPoller(RedisServer &server, std::list<QByteArray> keys, int timeout = 0, QObject *parent = 0);
        RedisListPoller(RedisServer &server, std::list<QByteArray> keys, PollTimeType pollTimeType, int timeout = 0, QObject *parent = 0);
        RedisListPoller(RedisServer &server, std::list<QByteArray> keys, PopPosition popPosition, int timeout = 0, QObject *parent = 0);
        RedisListPoller(RedisServer &server, std::list<QByteArray> keys, PollTimeType pollTimeType, PopPosition popPosition, int timeout = 0, QObject *parent = 0);
        ~RedisListPoller();

        // running control
        bool start();
        void stop(bool instantly = false);

        // getter / setter
        inline bool isRunning() { return !this->suspended && this->socket && this->socket->isReadable(); }
        inline std::list<QByteArray> keys() { return this->lstKeys; }
        inline void setKeys(std::list<QByteArray> keys) { this->lstKeys = keys; }
        inline int timeout() { return this->intTimeout; }
        inline void setTimeout(int timeout) { this->intTimeout = timeout; }
        inline PollTimeType pollTimeType() { return this->enumPollTimeType; }
        inline void setPollTimeType(PollTimeType pollTimeType) { this->enumPollTimeType = pollTimeType; }
        inline PopPosition popPosition() { return this->enumPopPosition; }
        inline void setPopPosition(PopPosition popPosition) { this->enumPopPosition = popPosition; }

    signals:
        void timeoutReached();
        void popped(QByteArray list, QByteArray value);

    private slots:
        bool pop();
        void handleResponse();
        bool acquireSocket();
        void releaseSocket();

    private:
        // constructor generalizer
        void init(RedisServer &server, std::list<QByteArray> keys, int timeout, PollTimeType pollTimeType = PollTimeType::Forever, PopPosition popPosition = PopPosition::Begin);

        int intTimeout;
        bool suspended = false;
        PollTimeType enumPollTimeType = PollTimeType::Forever;
        PopPosition enumPopPosition = PopPosition::Begin;
        std::list<QByteArray> lstKeys;
        RedisServer* server = 0;
        QTcpSocket* socket = 0;
        RedisServer::RedisRequest currentRequest;
};

#endif // REDISLISTPOLLER_H
