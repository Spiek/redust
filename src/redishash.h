#ifndef REDISMAP_H
#define REDISMAP_H

// redis
#include "redisvalue.h"
#include "redisinterface.h"

template< typename Key, typename Value >
class RedisHash
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
                this->queueElements = other.queueElements;

                // copy key data
                this->currentKey = other.currentKey;
                this->currentKeyVal = other.currentKeyVal;
                this->keyLoaded = other.keyLoaded;

                // copy value data
                this->currentValue = other.currentValue;
                this->currentValueVal = other.currentValueVal;
                this->valueLoaded = other.valueLoaded;

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
                this->loadEntry(true, false);
                return this->currentKeyVal;
            }
            NORM2VALUE(Value) value()
            {
                this->loadEntry(false, true);
                return this->currentValueVal;
            }
            NORM2VALUE(Value) operator *()
            {
                this->loadEntry(false, true);
                return this->currentValueVal;
            }
            NORM2POINTER(Value) operator ->()
            {
                this->loadEntry(false, true);
                return &this->currentValueVal;
            }

        private:
            iterator(RedisInterface* prmap, int pos, int cacheSize)
            {
                this->d = prmap;
                this->pos = pos;
                this->cacheSize = cacheSize;
                if(pos >= 0) this->forward(1);
            }

            // helper
            iterator& forward(int elements)
            {
                // reset current loaded key and value status
                if(elements > 0) {
                    this->keyLoaded = false;
                    this->valueLoaded = false;
                }

                // navigate elements forward
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
                this->d->simplifyHScan(&this->queueElements, this->cacheSize, this->posRedis, true, true, &this->posRedis);

                // if we couldn't get any items then we reached the end
                // Note: this could happend if we try to get data from an non-existing/empty key
                //       if this is the case then we call us again so that this iterator is set to end
                return this->queueElements.isEmpty() ? this->refillQueue() : true;
            }

            void loadEntry(bool key, bool value)
            {
                // load key (if not allready happened)
                if(key && !this->keyLoaded) {
                    this->currentKeyVal = RedisValue<Key>::deserialize(this->currentKey);
                    this->keyLoaded = true;
                }

                // load value (if not allready happened)
                if(value && !this->valueLoaded) {
                    this->currentValueVal = RedisValue<Value>::deserialize(this->currentValue);
                    this->valueLoaded = true;
                }
            }

            // data
            RedisInterface* d;
            int cacheSize;
            int posRedis = -1;
            int pos;
            QList<QByteArray> queueElements;
            QByteArray currentKey;
            QByteArray currentValue;
            NORM2VALUE(Key) currentKeyVal;
            NORM2VALUE(Value) currentValueVal;
            bool keyLoaded = false;
            bool valueLoaded = false;

        friend class RedisHash;
    };

    public:
        RedisHash(QString list, QString connectionName = "redis")
        {
            this->d = new RedisInterface(list, connectionName);
        }

        ~RedisHash()
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

        NORM2VALUE(Value) take(Key key, bool waitForAnswer = true, bool *removeResult = 0)
        {
            NORM2VALUE(Value) value = this->value(key);
            bool rResult = this->remove(key, waitForAnswer);
            if(removeResult) *removeResult = rResult;
            return value;
        }

        bool insert(Key key, Value value, bool waitForAnswer = false)
        {
            return this->d->insert(RedisValue<Key>::serialize(key),
                                   RedisValue<Value>::serialize(value), waitForAnswer);
        }

        NORM2VALUE(Value) value(Key key)
        {
            return RedisValue<Value>::deserialize(this->d->value(RedisValue<Key>::serialize(key)));
        }

        QList<NORM2VALUE(Key)> keys(int fetchChunkSize = -1)
        {
            // create result data list
            QList<NORM2VALUE(Key)> list;

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

        QList<NORM2VALUE(Value)> values(int fetchChunkSize = -1)
        {
            // create result data list
            QList<NORM2VALUE(Value)> list;

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

        QMap<NORM2VALUE(Key),NORM2VALUE(Value)> toMap(int fetchChunkSize = -1)
        {
            // create result data list
            QMap<NORM2VALUE(Key),NORM2VALUE(Value)> map;

            // fetch first result
            int pos = 0;
            QList<QByteArray> elements;
            this->d->fetchAll(&elements, fetchChunkSize, pos, &pos);

            // if caller want to select data in chunks so do it
            if(fetchChunkSize > 0) while(pos != 0) this->d->fetchAll(&elements, fetchChunkSize, pos, &pos);

            // deserialize the data
            for(auto itr = elements.begin(); itr != elements.end();itr += 2) {
                map.insert(RedisValue<Key>::deserialize(*itr), RedisValue<Value>::deserialize(*(itr + 1)));
            }

            // return map
            return map;
        }

        QHash<NORM2VALUE(Key),NORM2VALUE(Value)> toHash(int fetchChunkSize = -1)
        {
            // create result data list
            QHash<NORM2VALUE(Key),NORM2VALUE(Value)> hash;

            // fetch first result
            int pos = 0;
            QList<QByteArray> elements;
            this->d->fetchAll(&elements, fetchChunkSize, pos, &pos);

            // if caller want to select data in chunks so do it
            if(fetchChunkSize > 0) while(pos != 0) this->d->fetchAll(&elements, fetchChunkSize, pos, &pos);

            // deserialize the data
            for(auto itr = elements.begin(); itr != elements.end();itr += 2) {
                hash.insert(RedisValue<Key>::deserialize(*itr), RedisValue<Value>::deserialize(*(itr + 1)));
            }

            // return hash
            return hash;
        }

    private:
        RedisInterface* d;
};


#endif // REDISMAP_H
