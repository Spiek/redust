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

        typename std::pointer_traits<QList<NORM2VALUE(Key)>*>::element_type keys(int count = 0, int pos = 0, int *newPos = 0)
        {
            typename std::pointer_traits<QList<NORM2VALUE(Key)>*>::element_type list;
            QList<QByteArray> elements = this->d->keys(count, pos, newPos);
            for(auto itr = elements.begin(); itr != elements.end();) {
                list.append(RedisValue<Key>::deserialize(*itr));
                itr = elements.erase(itr);
            }
            return list;
        }

        typename std::pointer_traits<QList<NORM2VALUE(Value)>*>::element_type values(int count = 0, int pos = 0, int *newPos = 0)
        {
            typename std::pointer_traits<QList<NORM2VALUE(Value)>*>::element_type list;
            QList<QByteArray> elements = this->d->values(count, pos, newPos);
            for(auto itr = elements.begin(); itr != elements.end();) {
                list.append(RedisValue<Value>::deserialize(*itr));
                itr = elements.erase(itr);
            }
            return list;
        }

    private:
        RedisMapPrivate* d;
};


#endif // REDISMAP_H
