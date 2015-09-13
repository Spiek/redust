#ifndef REDISMAP_H
#define REDISMAP_H

// std lib
#include <typeinfo>

#include <QVariant>
#include <QMutex>

// own
#include "redismapconnectionmanager.h"

// protocolbuffer feature
#ifdef REDISMAP_SUPPORT_PROTOBUF
    #include <google/protobuf/message.h>
#endif

template< typename T, typename Enable = void >
class RedisValue
{
    // Don't allow data pointer types
    // Reason: Redismap don't store any keys or values localy, instead redismap just act as a proxy between the program and a remote redisserver which contains the actual data.
    //         Pointers would be too dangeours, because:
    //         * the data would not be touched by the Redismap and so maybe become out of date.
    //         * redismap is not able to handle the ownership of the pointers, qDeleteAll on the values would not working, because no data do exist localy!
    // Perfomance Tip:
    // * instead of using pointers you can define the Redismap with reference Types
    static_assert(!std::is_pointer<T>::value,
                  "Pointer types are not allowed in Redismap, because redismap is not storing any data!");

    public:
        static inline QVariant serialize(T value) {  return QVariant(value); }
        static inline T unserialize(QByteArray value) {  return QVariant(value).value<T>(); }
};

template<typename T>
class RedisValue<T, typename std::enable_if<std::is_base_of<google::protobuf::Message, T>::value && !std::is_pointer<T>::value>::type >
{
    public:
        static inline QString serialize(T value) { return QString::fromStdString(value.SerializeAsString()); }
        static inline T unserialize(QByteArray value) {
            T t;
            t.ParseFromArray(value.data(), value.length());
            return t;
        }
};

template<typename T>
class RedisValue<T&, typename std::enable_if<std::is_base_of<google::protobuf::Message, T>::value>::type >
{
    public:
        static inline QString serialize(T value) { return QString::fromStdString(value.SerializeAsString()); }
        static inline T unserialize(QByteArray value) {
            T t;
            t.ParseFromArray(value.data(), value.length());
            return t;
        }
};


class RedisMapPrivate
{
    public:
        RedisMapPrivate(QString list, QString connectionName = "redis")
        {
            this->redisList = list.toLocal8Bit() + ".";
            this->redisConnection = RedisMapConnectionManager::getReadyRedisConnection(connectionName);
        }

        bool insert(QVariant key, QVariant value, bool waitForAnswer = false)
        {
            // if connection is not okay, exit
            if(!RedisMapConnectionManager::checkRedisConnection(this->redisConnection)) return false;

            // simplify command params
            QByteArray baKey = key.toByteArray();
            QByteArray baValue = value.toByteArray();

            // Build and execute Command
            // SET key value
            // src: http://redis.io/commands/SET
            QByteArray* baList = &this->redisList;
            static const char* redisCmd = "*3\r\n$3\r\nSET\r\n$%d\r\n%s%s\r\n$%d\r\n%s\r\n";
            RedisMapPrivate::execRedisCommand(this->redisConnection->socket,
                                        // command
                                        redisCmd,

                                        // command params
                                        baKey.size() + baList->size(),
                                        baList->data(),
                                        baKey.data(),
                                        baValue.size(),
                                        baValue.data());

            // if user want to wait until redis server answers, so do it
            if(waitForAnswer) {
                this->redisConnection->socket->waitForBytesWritten();
                this->redisConnection->socket->waitForReadyRead();
                return RedisMapPrivate::checkRedisReturnValue(this->redisConnection->socket);
            }

            // otherwise everything is okay
            return true;
        }

    private:
        static inline bool checkRedisReturnValue(QAbstractSocket* socket)
        {
            return socket && socket->readAll().left(3) == "+OK";
        }

        static void execRedisCommand(QAbstractSocket* socket, const char* cmd, ...)
        {
            // check socket
            if(!socket) return;

            // init some static buffer vars
            static int sizeBuffer = REDISMAP_INIT_QUERY_BUFFER_CACHE_SIZE;
            static char* buffer = (char*)malloc(REDISMAP_INIT_QUERY_BUFFER_CACHE_SIZE);

            // only create one command at a time
            static QMutex mutex;
            QMutexLocker mlocker(&mutex);

            // create command in buffer (and resize buffer if needed!)
            va_list ap;
            va_start(ap, cmd);
            int len = vsnprintf(buffer, sizeBuffer, cmd, ap);
            if(len > sizeBuffer) {
                sizeBuffer = len;
                buffer = (char*)realloc(buffer, sizeBuffer);
                va_start(ap, cmd);
                len = vsnprintf(buffer, sizeBuffer, cmd, ap);
            }

            // write buffer to
            socket->write(buffer, len);

            // if we are reaching the max buffer cache, we decrease the size to max allowed
            if(sizeBuffer > REDISMAP_MAX_QUERY_BUFFER_CACHE_SIZE) {
                sizeBuffer = REDISMAP_MAX_QUERY_BUFFER_CACHE_SIZE;
                buffer = (char*)realloc(buffer, REDISMAP_MAX_QUERY_BUFFER_CACHE_SIZE);
            }
        }

    private:
        RedisMapConnectionManager::RedisConnection *redisConnection;
        QByteArray redisList;
};


template< typename Key, typename Value >
class RedisMap
{
    public:
        RedisMap(QString list, QString connectionName = "redis")
        {
            this->d = new RedisMapPrivate(list, connectionName);
        }
        ~RedisMap()
        {
            delete this->d;
        }

        bool insert(Key key, Value value, bool waitForAnswer = false)
        {
            // simplefy key and value
            QVariant vkey = RedisValue<Key>::serialize(key);
            QVariant vValue = RedisValue<Value>::serialize(value);

            // if something is wrong with key or value, exit
            if(!vkey.isValid() || !vValue.isValid()) {
                if(!vkey.isValid()) qDebug("Cannot handle Keytype %s", typeid(key).name());
                if(!vkey.isValid()) qDebug("Cannot handle Valuetype %s", typeid(value).name());
                return false;
            }

            return this->d->insert(vkey, vValue, waitForAnswer);
        }

    private:
        RedisMapPrivate* d;
};


#endif // REDISMAP_H
