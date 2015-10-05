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

template< typename T, typename Enable = void >
class RedisValue
{
    // Don't allow data pointer types
    // Reason: Redismap don't store any keys or values localy, instead redismap just act as a proxy between the program and a remote redisserver which contains the actual data.
    //         Pointers would be too dangeours, because:
    //         * the data would not be touched by the Redismap and so maybe become out of date.
    //         * redismap is not able to handle the ownership of the pointers, qDeleteAll on the values would not working, because no data do exist localy!
    // Perfomance Tip:
    // * instead of using pointers you can define the Redismap key and/or values as reference Types
    static_assert(!std::is_pointer<T>::value,
                  "Pointer types are not allowed in Redismap, because redismap is not storing any data!");

    public:
        static inline QByteArray serialize(T value) { return QVariant(value).value<QByteArray>(); }
        static inline T deserialize(QByteArray value) { return QVariant(value).value<typename std::remove_reference<T>::type>(); }
        static inline bool isSerializeable() {
            // init a dummy for type checking
            typename std::remove_reference<T>::type t = {};
            return QVariant(t).canConvert<QByteArray>();
        }
        static inline bool isDeserializeable() { return QVariant(QByteArray()).canConvert<typename std::remove_reference<T>::type>(); }
};

template<typename T>
class RedisValue<T, typename std::enable_if<std::is_arithmetic<T>::value && !std::is_pointer<T>::value && !std::is_reference<T>::value>::type >
{
    public:
        static inline QByteArray serialize(T value) {
            typename std::remove_reference<T>::type t = qToBigEndian<T>(value);
            return QByteArray((char*)(void*)&t, sizeof(typename std::remove_reference<T>::type));
        }
        static inline T deserialize(QByteArray value) {
            typename std::remove_reference<T>::type t;
            memcpy(&t, value.data(), sizeof(typename std::remove_reference<T>::type));
            t = qFromBigEndian<T>(t);
            return t;
        }
        static inline bool isSerializeable() { return true; }
        static inline bool isDeserializeable() { return true; }
};

#ifdef REDISMAP_SUPPORT_PROTOBUF
template<typename T>
class RedisValue<T, typename std::enable_if<std::is_base_of<google::protobuf::Message, T>::value && !std::is_pointer<T>::value>::type >
{
    public:
        static inline QByteArray serialize(T value) {
            QByteArray data;
            data.resize(value.ByteSize());
            value.SerializeToArray(data.data(), value.ByteSize());
            return data;
        }
        static inline T deserialize(QByteArray value) {
            T t;
            t.ParseFromArray(value.data(), value.length());
            return t;
        }

        // protocol buffer values are always serializeable
        static inline bool isSerializeable() { return true; }

        // protocol buffer values should always be unserializeable (depends on input data!)
        static inline bool isDeserializeable() { return true; }
};

template<typename T>
class RedisValue<T&, typename std::enable_if<std::is_base_of<google::protobuf::Message, T>::value>::type >
{
    public:
        static inline QByteArray serialize(T value) {
            QByteArray data;
            data.resize(value.ByteSize());
            value.SerializeToArray(data.data(), value.ByteSize());
            return data;
        }
        static inline T deserialize(QByteArray value) {
            T t;
            t.ParseFromArray(value.data(), value.length());
            return t;
        }

        // protocol buffer values are always serializeable
        static inline bool isSerializeable() { return true; }

        // protocol buffer values should always be unserializeable (depends on input data!)
        static inline bool isDeserializeable() { return true; }
};
#endif

#endif // REDISVALUE_H

