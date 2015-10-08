#ifndef REDISMAP_H
#define REDISMAP_H

// redis
#include "redisvalue.h"
#include "redismapprivate.h"

template< typename Key, typename Value >
class RedisMap
{
    public:
        RedisMap(QString list, QString connectionName = "redis")
        {
            this->d = new RedisMapPrivate(list, connectionName);

            // (de)serializeable check
            if(!RedisValue<Key>::isSerializeable())     qDebug("Cannot Serialize     Keytype %s", typeid(Key).name());
            if(!RedisValue<Key>::isDeserializeable())   qDebug("Cannot Deserialize   Keytype %s", typeid(Key).name());
            if(!RedisValue<Value>::isSerializeable())   qDebug("Cannot Serialize   Valuetype %s", typeid(Value).name());
            if(!RedisValue<Value>::isDeserializeable()) qDebug("Cannot Deserialize Valuetype %s", typeid(Value).name());
        }

        ~RedisMap()
        {
            delete this->d;
        }

        void clear(bool async = true)
        {
            this->d->clear(async);
        }

        bool insert(Key key, Value value, bool waitForAnswer = false)
        {
            return this->d->insert(RedisValue<Key>::serialize(key),
                                   RedisValue<Value>::serialize(value), waitForAnswer);
        }

        typename std::pointer_traits<NORM2VALUE(Value)*>::element_type value(Key key)
        {
            return RedisValue<Value>::deserialize(this->d->value(RedisValue<Key>::serialize(key)));
        }

    private:
        RedisMapPrivate* d;
};


#endif // REDISMAP_H
