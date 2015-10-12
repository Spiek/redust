#ifndef REDISVALUE_H
#define REDISVALUE_H

// std lib
#include <typeinfo>

// qtcore
#include <QVariant>
#include <QByteArray>
#include <QtEndian>

// protocolbuffer feature
#ifdef REDISMAP_SUPPORT_PROTOBUF
    #include <google/protobuf/message.h>
#endif

// template generation helpers
#define NORM2VALUE(T) typename std::remove_reference<typename std::remove_pointer<T>::type>::type
#define NORM2REFORVALUE(T) typename std::remove_pointer<T>::type

/* Parser for QVariant types */
template< typename T, typename Enable = void >
class RedisValue
{
    public:
        static inline QByteArray serialize(NORM2VALUE(T)* value) { return value ? QVariant(*value).value<QByteArray>() : QByteArray(); }
        static inline QByteArray serialize(NORM2REFORVALUE(T)  value) { return QVariant(value).value<QByteArray>(); }
        static inline NORM2VALUE(T) deserialize(QByteArray* value) { return value ? QVariant(*value).value<NORM2VALUE(T)>() : NORM2VALUE(T)(); }
        static inline NORM2VALUE(T) deserialize(QByteArray  value) { return QVariant(value).value<NORM2VALUE(T)>(); }
};

/* Parser for arithmetic types */
template<typename T>
class RedisValue<T, typename std::enable_if<std::is_arithmetic<NORM2VALUE(T)>::value>::type >
{
    public:
        static inline QByteArray serialize(NORM2VALUE(T)* value) {
            if(!value) return QByteArray();
            NORM2VALUE(T) t = qToBigEndian<NORM2VALUE(T)>(*value);
            return QByteArray((char*)(void*)&t, sizeof(NORM2VALUE(T)));
        }
        static inline QByteArray serialize(NORM2REFORVALUE(T) value) {
            NORM2VALUE(T) t = qToBigEndian<NORM2VALUE(T)>(value);
            return QByteArray((char*)(void*)&t, sizeof(NORM2VALUE(T)));
        }
        static inline NORM2VALUE(T) deserialize(QByteArray* value) {
            NORM2VALUE(T) t;
            if(!value) return t;
            memcpy(&t, value->data(), sizeof(NORM2VALUE(T)));
            t = qFromBigEndian<T>(t);
            return t;
        }
        static inline NORM2VALUE(T) deserialize(QByteArray value) {
            NORM2VALUE(T) t;
            memcpy(&t, value.data(), sizeof(NORM2VALUE(T)));
            t = qFromBigEndian<T>(t);
            return t;
        }
};

#ifdef REDISMAP_SUPPORT_PROTOBUF

/* Parser for google's protocolbuffer types */
template<typename T>
class RedisValue<T, typename std::enable_if<std::is_base_of<google::protobuf::Message, NORM2VALUE(T)>::value>::type >
{
    public:
        static inline QByteArray serialize(NORM2VALUE(T)* value) {
            if(!value) return QByteArray();
            QByteArray data;
            data.resize(value->ByteSize());
            value->SerializeToArray(data.data(), value->ByteSize());
            return data;
        }
        static inline QByteArray serialize(NORM2REFORVALUE(T) value) {
            QByteArray data;
            data.resize(value.ByteSize());
            value.SerializeToArray(data.data(), value.ByteSize());
            return data;
        }
        static inline NORM2VALUE(T) deserialize(QByteArray* value) {
            NORM2VALUE(T) t;
            if(!value) return t;
            t.ParseFromArray(value->data(), value->length());
            return t;
        }
        static inline NORM2VALUE(T) deserialize(QByteArray value) {
            NORM2VALUE(T) t;
            t.ParseFromArray(value.data(), value.length());
            return t;
        }
};
#endif

#endif // REDISVALUE_H

