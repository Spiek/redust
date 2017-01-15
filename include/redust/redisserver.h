#ifndef REDISMAPCONNECTIONMANAGER_H
#define REDISMAPCONNECTIONMANAGER_H

#include <QTcpSocket>
#include <QQueue>

class RedisServer : public QObject
{
    Q_OBJECT
    public:
        /*
         * Redis Response Data
         * - contains result data from the redis server
         */
        struct RedisResponseData
        {
            public:
                enum class Type {
                    Okay = 0,
                    Error = 1,

                    String = 2,
                    Integer = 3,
                    Array = 4,
                    ArrayList = 5
                };

                RedisResponseData(QTcpSocket* socket) : _type(RedisResponseData::Type::Okay), _socket(socket) { }

                // Type
                void type(RedisResponseData::Type type) { this->_type = type; }
                RedisResponseData::Type type() { return this->_type; }

                // Error
                bool hasError()  { return this->_type == RedisResponseData::Type::Error; }
                QString& error() { return this->_errorString; }
                void error(QString error)
                {
                    this->_type = RedisResponseData::Type::Error;
                    this->_errorString = error;
                }

                // String
                QByteArray& string() { return this->_string; }
                void string(QByteArray string)
                {
                    this->_type = RedisResponseData::Type::String;
                    this->_string = string;
                }

                // Integer
                int& integer() { return this->_integer; }
                void integer(int integer)
                {
                    this->_type = RedisResponseData::Type::Integer;
                    this->_integer = integer;
                }

                // Boolean
                bool boolean() {
                    return this->_type == RedisResponseData::Type::Integer ? this->integer() == 1 :
                           this->_type == RedisResponseData::Type::String  ? this->string() == "OK" : false;
                }

                // Array
                std::list<QByteArray>& array() { return this->_array; }
                void array(std::list<QByteArray> array)
                {
                    this->_type = RedisResponseData::Type::Array;
                    this->_array = array;
                }

                // Array List
                std::list<std::list<QByteArray>>& arrayList() { return this->_arrayList; }
                void arrayList(std::list<std::list<QByteArray>> arrayList)
                {
                    this->_type = RedisResponseData::Type::ArrayList;
                    this->_arrayList = arrayList;
                }

                // Future
                bool signalIgnored() { return this->_signalIgnored; }
                void signalIgnored(bool ignored)
                {
                    this->_signalIgnored = ignored;
                }

                // socket
                QTcpSocket* socket() { return this->_socket; }
                void socket(QTcpSocket* socket) { this->_socket = socket; }

            private:
                QByteArray _string;
                QString _errorString;
                int _integer = -1;
                std::list<QByteArray> _array;
                std::list<std::list<QByteArray>> _arrayList;
                bool _signalIgnored = 0;
                Type _type;
                QTcpSocket* _socket = 0;
        };
        typedef QSharedPointer<RedisResponseData> RedisResponse;

        /*
         * Redis Request Data
         * - contains request data from the user
         */
        struct RedisRequestData
        {
            RedisRequestData(QString error) : _response(new RedisResponseData(0)) { this->error(error); }
            RedisRequestData(QTcpSocket* socket) : _response(new RedisResponseData(socket)), _socket(socket) { }

            // Error
            bool hasError()  { return !this->_errorString.isEmpty(); }
            QString& error() { return  this->_errorString; }
            void error(QString error) { this->_errorString = error; }

            // socket
            QTcpSocket* socket() { return this->_socket; }

            // response
            bool hasReponse()  { return this->_response.data(); }
            RedisResponse response() { return this->_response; }
            void response(RedisResponse response) { this->_response = response; }

            // custom data
            bool hasCustomData()  { return !this->_customData.isNull(); }
            QVariant& customData() { return this->_customData; }
            void customData(QVariant customData) { this->_customData = customData; }

            // internal data
            RedisResponse _response;
            QTcpSocket* _socket = 0;
            QString _errorString;
            QVariant _customData;
        };
        typedef QSharedPointer<RedisRequestData> RedisRequest;

    signals:
        void redisResponseFinished(RedisServer::RedisRequest request, bool success);

    public:
        enum class Position {
            Begin,
            End
        };
        enum class ConnectionType {
            WriteOnly,
            ReadWrite,
            Blocked
        };
        enum class RequestType {
            WriteOnly,
            WriteOnlyBlocked,
            Syncron,
            Asyncron,
            PipeLine
        };

        // Con/Decons
        RedisServer(QString redisServer = "localhost", qint16 redisPort = 6379);
        ~RedisServer();

        // Redis server connection handling
        bool initConnections(bool readWrite = true, bool writeOnly = false, int blockedSockets = 0);
        QTcpSocket* requestConnection(RedisServer::ConnectionType type);
        void freeBlockedConnection(QTcpSocket *socket);

        // General Redis Protocol Implementation
        RedisRequest execRedisCommand(std::list<QByteArray> cmd, RequestType type, QTcpSocket *socket = 0);
        bool parseResponse(RedisRequest &request);
        int executePipeline(bool waitForBytesWritten = false);

        // General Redis Functions
        RedisRequest ping(QByteArray data = "", RequestType = RequestType::Asyncron);

        // Key-Value Redis Functions
        RedisRequest del(QByteArray key, RequestType type = RequestType::Syncron);
        RedisRequest exists(QByteArray key, RequestType type = RequestType::Syncron);
        RedisRequest keys(QByteArray pattern = "*", RequestType type = RequestType::Syncron);

        // List Redis Functions
        RedisRequest lpush(QByteArray key, QByteArray value, RequestType type = RequestType::Asyncron);
        RedisRequest lpush(QByteArray key, std::list<QByteArray> values, RequestType type = RequestType::Asyncron);
        RedisRequest rpush(QByteArray key, QByteArray value, RequestType type = RequestType::Asyncron);
        RedisRequest rpush(QByteArray key, std::list<QByteArray> values, RequestType type = RequestType::Asyncron);
        bool blpop(QTcpSocket *socket, std::list<QByteArray> lists, int timeout = 0, RequestType type = RequestType::WriteOnly);
        bool brpop(QTcpSocket *socket, std::list<QByteArray> lists, int timeout = 0, RequestType type = RequestType::WriteOnly);
        RedisRequest llen(QByteArray key, RequestType type = RequestType::Syncron);

        // Hash Redis Functions
        RedisRequest hlen(QByteArray list, RequestType type = RequestType::Syncron);
        RedisRequest hset(QByteArray list, QByteArray key, QByteArray value, RequestType type = RequestType::Asyncron);
        RedisRequest hsetnx(QByteArray list, QByteArray key, QByteArray value, RequestType type = RequestType::Asyncron);
        RedisRequest hmset(QByteArray list, std::list<QByteArray> keys, std::list<QByteArray> values, RequestType type = RequestType::Asyncron);
        RedisRequest hmset(QByteArray list, std::map<QByteArray, QByteArray> entries, RequestType type = RequestType::Asyncron);
        RedisRequest hexists(QByteArray list, QByteArray key, RequestType type = RequestType::Syncron);
        RedisRequest hdel(QByteArray list, QByteArray key, RequestType type = RequestType::Asyncron);
        RedisRequest hget(QByteArray list, QByteArray key, RequestType type = RequestType::Syncron);
        RedisRequest hgetall(QByteArray list, RequestType type = RequestType::Asyncron);
        RedisRequest hmget(QByteArray list, std::list<QByteArray> keys, RequestType type = RequestType::Asyncron);
        RedisRequest hstrlen(QByteArray list, QByteArray key, RequestType type = RequestType::Asyncron);
        RedisRequest hkeys(QByteArray list, RequestType type = RequestType::Asyncron);
        RedisRequest hvals(QByteArray list, RequestType type = RequestType::Asyncron);

    private:
        // very fast implementation of integer places counting
        // src: http://stackoverflow.com/a/1068937
        static inline int numIntPlaces(int n) {
            if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
            if (n < 10) return 1;
            if (n < 100) return 2;
            if (n < 1000) return 3;
            if (n < 10000) return 4;
            if (n < 100000) return 5;
            if (n < 1000000) return 6;
            if (n < 10000000) return 7;
            if (n < 100000000) return 8;
            if (n < 1000000000) return 9;
            /*      2147483647 is 2^31-1 - add more ifs as needed
               and adjust this final return as well. */
            return 10;
        }

    private slots:
        void handleRedisResponse();

    private:
        // connection queues
        QTcpSocket* socketWriteOnly = 0;
        QTcpSocket* socketReadWrite = 0;
        QQueue<QTcpSocket*> lstBlockedSockets;

        // redis connection data
        QString strRedisConnectionHost;
        quint16 intRedisConnectionPort;

        // pipeline data
        QQueue<RedisServer::RedisRequest> pendingRequests;
        QQueue<RedisServer::RedisRequest> pendingPipelineRequests;
        QByteArray pendingPipelineData;
};

#endif // REDISMAPCONNECTIONMANAGER_H
