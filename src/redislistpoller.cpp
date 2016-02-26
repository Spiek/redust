#include "recotec/redislistpoller.h"

RedisListPoller::RedisListPoller(RedisServer &server, QList<QByteArray> keys, RedisInterface::PopDirection popDirection, int timeout, QObject *parent) : QObject(parent)
{
    // save keys in deserialized form
    this->intTimeout = timeout;
    this->lstKeys = keys;
    this->popDirection = popDirection;
    this->server = &server;
}

RedisListPoller::~RedisListPoller()
{
    // stop instantly
    this->stop(true);
}

bool RedisListPoller::start()
{
    // reset suspend
    this->suspended = false;

    // acquire socket
    if(!this->socket) {
        this->socket = this->server->requestConnection(RedisServer::ConnectionType::Blocked);
        if(this->socket) this->connect(this->socket, &QTcpSocket::readyRead, this, &RedisListPoller::handleResponse);
        else return false;
    }

    // run blpop and return result
    return RedisInterface::bpop(this->socket, this->lstKeys, this->popDirection, this->intTimeout);
}

void RedisListPoller::stop(bool instantly)
{
    // to stop instantly we have to kill the socket
    if(instantly) {
        if(socket) this->socket->deleteLater();
        this->socket = 0;
    }

    // otherwise we set the suspended flag so that after the next received data we don't start over
    else {
        this->suspended = true;
    }
}

void RedisListPoller::handleResponse()
{
    // exit if socket is not valid
    if(!this->socket) return;

    // parse result and exit on fail
    QList<QByteArray> result;
    if(!RedisInterface::parseResponse(this->socket, 0, &result)) return;

    // if no element could be popped, timeout reached
    if(result.count() == 1 && result.first().isNull()) emit this->timeout();

    // otherwise inform outside world about popped element
    else if(result.count() == 2) emit this->popped(result.first(), result.last());

    // if suspended flag is set, free the socket and don't start over
    if(this->suspended) {
        this->disconnect(this->socket);
        this->server->freeBlockedConnection(this->socket);
        this->socket = 0;
    }

    // otherwise just start over
    else this->start();
}
