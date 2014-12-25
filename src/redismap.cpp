#include "redismap.h"

QMap<QString, RedisMapConnectionManager::RedisConnection*> RedisMapConnectionManager::mapRedisConnections = QMap<QString, RedisMapConnectionManager::RedisConnection*>();
