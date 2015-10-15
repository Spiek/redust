#ifndef REDISMAP_H
#define REDISMAP_H

// redis
#include "redisvalue.h"
#include "redismapprivate.h"

template< typename Key, typename Value >
class RedisMap
{
    class iterator
    {
        public:
            // self operator overloadings
            iterator& operator =(iterator other)
            {
                this->d = other.d;
                this->pos = other.pos;
                this->posRedis = other.posRedis;
                this->cacheSize = other.cacheSize;
                this->currentKey = other.currentKey;
                this->currentValue = other.currentValue;
                this->queueElements = other.queueElements;
                return *this;
            }
            iterator& operator ++()
            {
                return this->forward(1);
            }
            iterator& operator ++(int elements)
            {
                Q_UNUSED(elements);
                return this->operator ++();
            }
            iterator& operator +=(int elements)
            {
                return this->forward(elements);
            }

            // copy operator overloadings
            iterator operator +(int elements)
            {
                iterator inew = *this;
                inew.forward(elements);
                return inew;
            }

            // comparing operator overloadings
            bool operator ==(iterator other)
            {
                return this->pos == other.pos;
            }
            bool operator !=(iterator other)
            {
                return !this->operator==(other);
            }

            // key value overloadings
            NORM2VALUE(Key) key()
            {
                return RedisValue<Key>::deserialize(this->currentKey);
            }
            NORM2VALUE(Value) value()
            {
                return RedisValue<Value>::deserialize(this->currentValue);
            }
            NORM2VALUE(Value) operator *()
            {
                return RedisValue<Value>::deserialize(this->currentKey);
            }
            NORM2VALUE(Value) operator ->()
            {
                return RedisValue<Value>::deserialize(this->currentValue);
            }

        private:
            iterator(RedisMapPrivate* prmap, int pos, int cacheSize)
            {
                this->d = prmap;
                this->pos = pos;
                this->cacheSize = cacheSize;
                if(pos >= 0) this->forward(1);
            }

            // helper
            iterator& forward(int elements)
            {
                for(int i = 0; i < elements; i++) {
                    if(!this->refillQueue()) return *this;

                    // save current key and value
                    this->currentKey = this->queueElements.takeFirst();
                    this->currentValue = this->queueElements.takeFirst();
                }
                return *this;
            }

            bool refillQueue()
            {
                // if queue is not empty, don't refill
                if(!this->queueElements.isEmpty()) return true;

                // if we reach the end of the redis position, set current pos to -1 and exit
                if(this->posRedis == 0) this->pos = -1;
                if(this->pos == -1) return false;

                // if redis pos is set to -1 (it's an invalid position) set position to begin
                // Note: this is a little hack for initialiating because redis start and end value are both 0
                if(this->posRedis == -1) this->posRedis = 0;

                // refill queue
                this->queueElements = this->d->simplifyHScan(this->cacheSize, this->posRedis, true, true, &this->posRedis);

                // if we couldn't get any items then we reached the end
                // Note: this could happend if we try to get data from an non-existing/empty key
                //       if this is the case then we call us again so that this iterator is set to end
                return this->queueElements.isEmpty() ? this->refillQueue() : true;
            }

            // data
            RedisMapPrivate* d;
            int cacheSize;
            int posRedis = -1;
            int pos;
            QList<QByteArray> queueElements;
            QByteArray currentKey;
            QByteArray currentValue;

        friend class RedisMap;
    };

    public:
        RedisMap(QString list, QString connectionName = "redis")
        {
            this->d = new RedisMapPrivate(list, connectionName);
        }

        ~RedisMap()
        {
            delete this->d;
        }

        iterator begin(int cacheSize = 100)
        {
            return iterator(this->d, 0, cacheSize);
        }

        iterator end(int cacheSize = 100)
        {
            return iterator(this->d, -1, cacheSize);
        }

        void clear(bool async = true)
        {
            return this->d->clear(async);
        }

        int count()
        {
            return this->d->count();
        }

        bool empty()
        {
            return this->isEmpty();
        }

        bool isEmpty()
        {
            return !this->d->count();
        }

        bool exists(Key key)
        {
            return this->d->contains(RedisValue<Key>::serialize(key));
        }

        bool exists()
        {
            return this->d->exists();
        }

        bool remove(Key key, bool waitForAnswer = true)
        {
            return this->d->remove(RedisValue<Key>::serialize(key), waitForAnswer);
        }

        Value take(Key key, bool waitForAnswer = true, bool *removeResult = 0)
        {
            Value value = this->value(key);
            bool rResult = this->remove(key, waitForAnswer);
            if(removeResult) *removeResult = rResult;
            return value;
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

        typename std::pointer_traits<QList<NORM2VALUE(Key)>*>::element_type keys(int fetchChunkSize = -1)
        {
            // create result data list
            typename std::pointer_traits<QList<NORM2VALUE(Key)>*>::element_type list;

            // fetch first result
            int pos = 0;
            QList<QByteArray> elements;
            this->d->fetchKeys(&elements, fetchChunkSize, pos, &pos);

            // if caller want to select data in chunks so do it
            if(fetchChunkSize > 0) while(pos != 0) this->d->fetchKeys(&elements, fetchChunkSize, pos, &pos);

            // deserialize the data
            for(auto itr = elements.begin(); itr != elements.end(); itr = elements.erase(itr)) {
                list.append(RedisValue<Key>::deserialize(*itr));
            }

            // return list
            return list;
        }

        typename std::pointer_traits<QList<NORM2VALUE(Value)>*>::element_type values(int fetchChunkSize = -1)
        {
            // create result data list
            typename std::pointer_traits<QList<NORM2VALUE(Key)>*>::element_type list;

            // fetch first result
            int pos = 0;
            QList<QByteArray> elements;
            this->d->fetchValues(&elements, fetchChunkSize, pos, &pos);

            // if caller want to select data in chunks so do it
            if(fetchChunkSize > 0) while(pos != 0) this->d->fetchValues(&elements, fetchChunkSize, pos, &pos);

            // deserialize the data
            for(auto itr = elements.begin(); itr != elements.end(); itr = elements.erase(itr)) {
                list.append(RedisValue<Key>::deserialize(*itr));
            }

            // return list
            return list;
        }

    private:
        RedisMapPrivate* d;
};


#endif // REDISMAP_H
