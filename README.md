## ReCoTeC - Redis Container Templates for C++

ReCoTeC provides easy plattform intependant access for Qt-C++ applications   
to access data stored in Redis using C++\-Container-Class-Templates.   
All Template Classes are acting as direct proxies to Redis, so nothing is saved or processed locally.   

### Supported Types
ReCoTeC are able to handle the following types as Key or Value type:   
 - All Types which QVariant is able from/to serialize to QByteArray
 - All classes which inherits from google::protobuf::Message
 - Integral Types are able to become serialized into binary or string format

### Available Container Types
ReCoTeC currently supports the following container types:
 - RedisHash< Key, Value > - Hash-table-based dictionary
	 - act as [Redis hash][redis-hashes-explained]
	 - reimplement all possible public function signatures of [QHash][qhash-public-signature]
	 - O(1) lookups on key (by redis)
	 - unsorted

### How to compile?
**Static:**  
In your Project file:
```qmake
include(recotec.pri)
```
**Dynamic:**  
1. qmake recotec.pro
2. make
3. make install
4. add the following to your pro file:   
```qmake
LIBS += -lrecotec
```
### How to use?
Here an example use case:
```c++
#include <QString>
#include <QDebug>
#include <recotec/redishash.h>

int main()
{
    // add redis connection
    RedisConnectionManager::addConnection("redis", "127.0.0.1", 6379);

    // create a redis hash which uses the "redis" connection pool
    // without trying to binarize the key or the value
    RedisHash<qint16, QString> rhash("MYREDISKEY", false, false, "redis");

    // insert values
    rhash.insert(123, "This is a Test Insert 1");
    rhash.insert(956, "This is a Test Insert 2");

    // get values
    qDebug("%s", qPrintable(rhash.value(123));
    qDebug("%s", qPrintable(rhash.value(956));

    // c++11 iteration
    qDebug("c++11 Iteration");
    for(qint16 key : rhash.keys()) {
        qDebug("%i -> %s", key, qPrintable(rhash.value(key)));
    }

    // C++99 iteration
    qDebug("c++99 Iteration");
    for(auto itr = rhash.begin(); itr != rhash.end(); itr++) {
        qDebug("%i -> %s", itr.key(), qPrintable(itr.value()));
    }

    // delete values
    rhash.remove(123);

    // count elements
    qDebug() << rhash.count();

    // take element
    qDebug("%s", qPrintable(rhash.take(956));
}
```
Prints:
```
This is a Test Insert 1
This is a Test Insert 2
c++11 Iteration
123 -> This is a Test Insert 1
956 -> This is a Test Insert 2
c++99 Iteration
123 -> This is a Test Insert 1
956 -> This is a Test Insert 2
1
This is a Test Insert 2
```

... and generates the following Redis Command sequence:

Command  | List | Parameter 1 | Parameter 2 | Parameter 3
:-------- | :------- | :--- | :--- | :---
HSET | MYREDISKEY | 123 | This is a Test Insert 1
HSET | MYREDISKEY | 956 | This is a Test Insert 2
HGET | MYREDISKEY | 123
HGET | MYREDISKEY | 956
HKEYS | MYREDISKEY | 
HGET | MYREDISKEY | 123
HGET | MYREDISKEY | 956
HSCAN | MYREDISKEY | 0 | COUNT | 100
HDEL | MYREDISKEY | 123
HLEN | MYREDISKEY | 
HGET | MYREDISKEY | 956
HDEL | MYREDISKEY | 956


[//]: # 

[redis-hashes-explained]: <http://redis.io/topics/data-types#hashes>
[qhash-public-signature]: <http://doc.qt.io/qt-5/qhash.html#public-functions>

