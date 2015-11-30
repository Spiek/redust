## ReCoTeC - Redis Container Templates for C++

ReCoTeC provides easy plattform intependant access for Qt-C++ applications
to access data stored in Redis using C++-Container-Class-Templates.

ReCoTeC don't save anything locally, all actions are forwarded directly to redis.

###Supported Types

ReCoTeC are able to handle the following Types as Key or Value:

 - All Types which QVariant is able to serialize to QByteArray
 - All classes which inherits from google::protobuf::Message
 - Integral Types are able to become serialized in binary or as string

ReCoTeC currently supports the following container types:

 - RedisHash< Key, Value > - Hash-table-based dictionary
	 - function signatures taken from Qt's QHash
	 - O(1) lookups on key
	 - unsorted

### How to compile?

**Static:**

In your Project file:
```qmake
include(recotec.pri)
```
**Dynamic:**

1. compile recotec.pro
2. make the compiled library available to your qt build system
3. add the following to your pro file:
```qmake
LIBS += -lrecotec
```
### How to use?
Here an example use case:
```c++
#include <QString>
#include <QDebug>
#include <redishash.h>

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
    qDebug() << rhash.value(123);
    qDebug() << rhash.value(956);

    // c++11 iteration
    qDebug("c++11 Iteration");
    for(qint16 key : rhash.keys()) {
        qDebug("%i -> %s", key, qPrintable(rhash.value(key)));
    }

    // c++ iteration
    qDebug("c++ Iteration");
    for(auto itr = rhash.begin(); itr != rhash.end(); itr++) {
        qDebug("%i -> %s", itr.key(), qPrintable(itr.value()));
    }

    // delete values
    rhash.remove(123);

    // count elements
    qDebug() << rhash.count();

    // take element
    qDebug() << rhash.take(956);
}
```
Prints:
```
"This is a Test Insert 1"
"This is a Test Insert 2"
c++11 Iteration
123 -> This is a Test Insert 1
956 -> This is a Test Insert 2
c++ Iteration
123 -> This is a Test Insert 1
956 -> This is a Test Insert 2
1
"This is a Test Insert 2"
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
