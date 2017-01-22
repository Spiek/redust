#include "redust/redislistpoller.h"

RedisListPoller::RedisListPoller(RedisServer &server, std::list<QByteArray> keys, int timeout, PollTimeType pollTimeType, QObject *parent) : QObject(parent)
{
    // save keys in deserialized form
    this->intTimeout = timeout;
    this->lstKeys = keys;
    this->server = &server;
    this->enumPollTimeType = pollTimeType;
}

RedisListPoller::~RedisListPoller()
{
    // stop instantly
    this->stop(true);
}

bool RedisListPoller::start()
{
    // if is allready running, exit success
    if(this->isRunning()) return true;

    // oterwise pop
    return this->pop();
}

void RedisListPoller::stop(bool instantly)
{
    // to stop instantly we have to free the socket
    if(instantly) this->releaseSocket();

    // otherwise we set the suspended flag so that after the next received data we don't start over
    else this->suspended = true;
}

bool RedisListPoller::pop()
{
    // reset suspend
    this->suspended = false;

    // acquire socket, and exit on fail
    if(!this->acquireSocket()) return false;

    // run blpop and return result
    this->currentRequest = this->server->blpop(this->socket, this->lstKeys, this->intTimeout);
    return !this->currentRequest->hasError();
}

void RedisListPoller::handleResponse()
{
    // exit if socket is not valid
    if(!this->socket) return;

    // parse result and exit on fail
    if(!this->server->parseResponse(this->currentRequest) || this->currentRequest->hasError()) return;
    std::list<QByteArray> result = this->currentRequest->response()->array();

    // if no element could be popped, timeout reached
    if(result.size() == 1 && result.front().isNull()) {
        // if user only want to loop until timeout reached, so suspend
        if((this->enumPollTimeType & PollTimeType::UntilTimeout) == PollTimeType::UntilTimeout) this->suspended = true;
        emit this->timeoutReached();
    }

    // otherwise inform outside world about popped element
    else if(result.size() == 2) {
        // if user only want to loop until first pop, so suspend
        if((this->enumPollTimeType & PollTimeType::UntilFirstPop) == PollTimeType::UntilFirstPop) this->suspended = true;
        emit this->popped(result.front(), result.back());
    }

    // if suspended flag is set, free the socket instantly and don't start over
    if(this->suspended) this->releaseSocket();

    // otherwise just start over
    else this->pop();
}

bool RedisListPoller::acquireSocket()
{
    // if have allready an acquired socket, exit
    if(this->socket) return true;

    // otherwise acquire one (and return false on error)
    this->socket = this->server->requestConnection(RedisServer::ConnectionType::Blocked);
    if(this->socket) this->connect(this->socket, &QTcpSocket::readyRead, this, &RedisListPoller::handleResponse);
    else return false;

    // socket was successfull acquired
    return true;
}

void RedisListPoller::releaseSocket()
{
    // exit if we have no socket to release
    if(!this->socket) return;

    // release socket
    this->disconnect(this->socket);
    this->server->freeBlockedConnection(this->socket);
    this->socket = 0;
}
