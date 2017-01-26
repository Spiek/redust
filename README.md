# Redis Usability Simplifier ToolKit for Qt

RedUST provides some usability simplifier for accessing redis via Qt.

Philosophy:  
The Idea is to provide a very easy high level api to simplify atomic tasks using a redis server.   
All tools provided by RedUST work directly with redis, so they work atomic accross threads, applications and networks.  
All data becomes accessed/changed/deleted direct in redis on demand.

----------

## Redis Templates:
Redis Templates are simplify the access to redis, by providing easy to use template classes.  
All Redis Templates are able to handle the following Template types as Key or Value type:

 - All Types which QVariant is able from/to serialize to QByteArray
 - All classes which inherits from google::protobuf::Message
 - All Integral Types are able to become serialized into binary or string format

<details><summary>**Redis Hash** - [QHash][qhash-public-signature] like Redis interface </summary>

RedisHash< Key, Value > - Hash-table-based dictionary
The following characteristics apply:
 - get/set all data in a [Redis hash][redis-hashes-explained] in the redis server
 - reimplement all possible public function signatures of [QHash][qhash-public-signature]
 - O(1) lookups on key (by redis)
 - unsorted
 
Example:
```c++
#include <redust/RedisHash>

int main()
{
    // create redis connection
    RedisServer server("127.0.0.1", 6379);

    // create a redis hash with qint16 type as key, without trying to binarize the key or the value
    RedisHash<qint16, QString> rhash(server, "MYREDISKEY", false, false);

    // insert values
    rhash.insert(123, "This is a Test Insert 1", RedisServer::RequestType::Syncron);
    rhash.insert(956, "This is a Test Insert 2", RedisServer::RequestType::Syncron);

    // get values
    qDebug("%s", qPrintable(rhash.value(123)));
    qDebug("%s", qPrintable(rhash.value(956)));

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
    qDebug("%s", qPrintable(rhash.take(956)));
	
	return 0;
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

</details>

----------

## Redis Tools
RedUST has additional Tools to simplify different tasks


<details><summary>**RedisServer** - redis server network/command interface</summary>

The RediServer act as redis interface, it handles redis network connections and gives easy high level access to all kind of redis commands.

This Example demonstrate some of these functions:
```c++
#include <QCoreApplication>
#include <redust/RedisServer>

int main(int argc, char** argv)
{
    QCoreApplication a(argc, argv);

    // create redis connection
    RedisServer server("127.0.0.1", 6379);

    ///
    /// List Function Examples
    ///

    // delete list
    qDebug("The list mylist has%s been deleted",
           server.del("mylist")->response()->integer() == 0 ? " NOT" : "");

    // async multi list insertation
    server.rpush("mylist", {"first", "second"}, RedisServer::RequestType::Asyncron);

    // syncron list insertation (including result)
    qDebug("The list mylist has after insert %i-entries",
           server.lpush("mylist", "third", RedisServer::RequestType::Syncron)->response()->integer());

    // pipeline inseration
    RedisServer::RedisRequest request = server.lpush("mylist", "fourth", RedisServer::RequestType::PipeLine);
    server.rpush("mylist", "fifth", RedisServer::RequestType::PipeLine);

    // syncron pipeline execution (async is default!)
    qDebug("%i pipeline commands executed",
           server.executePipeline(RedisServer::RequestType::Syncron));

    // access redis server results
    qDebug("RedisResult of \"lpush mylist third\" is %i",
           request->response()->integer());

    // get list count
    qDebug("Redis list mylist has currently %i entries",
           server.llen("mylist")->response()->integer());


    ///
    /// Hash Function Examples
    ///

    // try to delete the hashlist "myhash"
    qDebug("The hashlist myhash has%s been deleted",
           server.del("myhash")->response()->integer() == 0 ? " NOT" : "");

    // multi syncron hash insert
    server.hmset("myhash", {
                            {"myfirstkey", "myfirstvalue"},
                            {"myfirstkey", "myfirstvalue"}
                           }, RedisServer::RequestType::Syncron);

    // access value
    qDebug("myfirstkey has value %s",
           qPrintable(server.hget("myhash", "myfirstkey")->response()->string()));

    return a.exec();
}
```
Prints:
```
The list mylist has been deleted
The list mylist has after insert 1-entries
2 pipeline commands executed
RedisResult of "lpush mylist third" is 4
Redis list mylist has currently 5 entries
The hashlist myhash has been deleted
myfirstkey has value myfirstvalue
```
... and generates the following Redis Command sequence:

Command  | Parameter 1 | Parameter 2 | Parameter 3
:-------- | :------- | :--- | :--- 
DEL | mylist
LPUSH | mylist | third
RPUSH | mylist | first | second
LPUSH | mylist | fourth
RPUSH | mylist | fifth
LLEN | mylist
DEL | myhash
HMSET | myhash | myfirstkey | myfirstvalue
HGET | myhash | myfirstkey
</details>

<details><summary>**RedisListPoller** - async blocking pop</summary>

The Redis list poller provide a async [BLPOP][blpop-explained] or [BRPOP][brpop-explained] for new elements in giving lists.  
As soon as redis send an element back to the client the class emit the popped()-Signal with the popped element,  
or the emit the timeout()-Signal (if timeout reached).

The RedisListPoller works directly with redis, so it works atomic accross thread, applications and networks.

A possible use Case:  
The RedisListPoller can be used to provide multiple application atomic (in a round robin way) with events.

Example:   
```c++
#include <QCoreApplication>
#include <redust/RedisListPoller>

int main(int argc, char** argv)
{
    // init qt application
    QCoreApplication a(argc, argv);
    
    // create redis connection
    RedisServer server("127.0.0.1", 6379);
    
    // push data into redis lists
    // Note: we force a syncron execution here because we want that redis insert the data before we start polling!
    qDebug("Now we have %i elements in the list list1", server.rpush("list1", {"val1", "val2"}, RedisServer::RequestType::Syncron)->response()->integer());
    qDebug("Now we have %i elements in the list list2", server.rpush("list2", {"val3", "val4"}, RedisServer::RequestType::Syncron)->response()->integer());

    // start list poller for lists "list1" and "list2" with a timeout of 1 second until timeout reached
    RedisListPoller listPoller(server, {"list1", "list2"});
    QObject::connect(&listPoller, &RedisListPoller::popped, [](QByteArray list, QByteArray value) {
        qDebug("Popped %s.%s", qPrintable(list), qPrintable(value));
    });
    listPoller.start();
    
    // start event loop
    return a.exec();
}
```
Prints:
```
Now we have 2 elements in the list list1
Now we have 2 elements in the list list2
Popped list1.val1
Popped list1.val2
Popped list2.val3
Popped list2.val4
```
... and generates the following Redis Command sequence:

Command  | Parameter 1 | Parameter 2 | Parameter 3
:-------- | :------- | :--- | :--- 
RPUSH | list1 | val1 | val2
RPUSH | list2 | val3 | val4
BLPOP | list1 | list2 | 1
BLPOP | list1 | list2 | 1
BLPOP | list1 | list2 | 1
BLPOP | list1 | list2 | 1
BLPOP | list1 | list2 | 1
</details>

----------

### Installation

<details><summary>**How to compile the library?**</summary>

**Static:**  
Add to your Project file:
```qmake
include(redust.pri)
```
**Dynamic:**  
```
qmake redust.pro
make
make install
```
add the following to your pro file:
```qmake
LIBS += -lredust
```
</details>



[//]: # 

[redis-hashes-explained]: <http://redis.io/topics/data-types#hashes>
[qhash-public-signature]: <http://doc.qt.io/qt-5/qhash.html#public-functions>
[blpop-explained]: <http://redis.io/commands/BLPOP>
[brpop-explained]: <http://redis.io/commands/BRPOP>
