#ifndef TYPESERIALIZER_H
#define TYPESERIALIZER_H

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

// Template to normalize T to Value Type
template< typename T>
struct ValueType
{ typedef T type; };

template< typename T>
struct ValueType<T*>
{  typedef typename std::remove_pointer<T>::type type; };

template< typename T>
struct ValueType<T&>
{  typedef typename std::remove_reference<T>::type type; };


// Template to normalize T to Value or Reference Type
template< typename T>
struct ValueRefType
{ typedef T type; };

template< typename T>
struct ValueRefType<T*>
{  typedef typename std::remove_pointer<T>::type type; };

// Type normalisations helper macros
#define NORM2VALUE(T) typename ValueType<T>::type
#define NORM2REFORVALUE(T) typename ValueRefType<T>::type
#define NORM2POINTER(T) typename ValueType<T>::type*

/* Parser for QVariant and arithmetic types */
template< typename T, typename Enable = void >
class TypeSerializer
{
    public:
        static inline QByteArray serialize(NORM2VALUE(T)* value, bool binarize) {
            if(!value) return QByteArray();
            if(binarize && std::is_integral<NORM2VALUE(T)>::value) {
                NORM2VALUE(T) t = sizeof(NORM2VALUE(T)) == 1 ? *value : qToBigEndian<NORM2VALUE(T)>(*value);

                // remove all leading \0-chars from value (except last char!), to save some extra space in redis
                // Note: we can do that, because we are serialize integral type to bigEndian and so the starting \0-chars can be ignored!
                // Example: big endian data (for an int16) 00FF (Hex) -becomes-> FF (Hex)
                char* data = (char*)(void*)&t;
                while((std::size_t)(data - (char*)(void*)&t) < sizeof(NORM2VALUE(T)) - 1 && !*data) data++;

                return QByteArray(data, sizeof(NORM2VALUE(T)) - (data - (char*)(void*)&t));
            } else return QVariant(*value).value<QByteArray>();
        }
        static inline QByteArray serialize(NORM2REFORVALUE(T) value, bool binarize) { return TypeSerializer<T>::serialize(&value, binarize); }
        static inline NORM2VALUE(T) deserialize(QByteArray* value, bool binarize) {
            NORM2VALUE(T) t;
            if(!value) return t;
            if(binarize && std::is_integral<NORM2VALUE(T)>::value) {
                // copy available data to end of t and set all data before to \0 (this reverts the byte saving in serialisation)
                // Example: big endian data (for an int16) FF (Hex) -becomes-> 00FF (Hex)
                memset((char*)&t, 0, sizeof(NORM2VALUE(T)) - value->length());
                memcpy((char*)&t + qMax((size_t)0, sizeof(NORM2VALUE(T)) - value->length()), value->data(), qMin(sizeof(NORM2VALUE(T)), (size_t)value->length()));
                if(sizeof(NORM2VALUE(T)) > 1) t = qFromBigEndian<T>(t);
            } else t = QVariant(*value).value<NORM2VALUE(T)>();
            return t;
        }
        static inline NORM2VALUE(T) deserialize(QByteArray  value, bool binarize) { return TypeSerializer<T>::deserialize(&value, binarize); }
};

#ifdef REDISMAP_SUPPORT_PROTOBUF

/* Parser for google's protocolbuffer types */
template<typename T>
class TypeSerializer<T, typename std::enable_if<std::is_base_of<google::protobuf::Message, NORM2VALUE(T)>::value>::type >
{
    public:
        static inline QByteArray serialize(NORM2VALUE(T)* value, bool binarize) {
            Q_UNUSED(binarize);
            QByteArray data;
            if(!value) return data;
            data.resize(value->ByteSize());
            value->SerializeToArray(data.data(), value->ByteSize());
            return data;
        }
        static inline QByteArray serialize(NORM2REFORVALUE(T) value, bool binarize) { return TypeSerializer<T>::serialize(&value, binarize); }
        static inline NORM2VALUE(T) deserialize(QByteArray* value, bool binarize) {
            Q_UNUSED(binarize);
            NORM2VALUE(T) t;
            if(!value) return t;
            if(!value->isEmpty()) t.ParseFromArray(value->data(), value->length());
            return t;
        }
        static inline NORM2VALUE(T) deserialize(QByteArray value, bool binarize) { return TypeSerializer<T>::deserialize(&value, binarize); }
};
#endif

#endif // TYPESERIALIZER_H

