#include <drogon/HttpController.h>
#include <drogon/WebSocketController.h>
#include <mariadb/conncpp.hpp>
#include <nlohmann/json.hpp>
#include "../macro.h"
#include "../utils.h"
#include "stdio.h"

using namespace drogon;
using namespace sql;
using CallBackType = std::function<void(const HttpResponsePtr &)>;
using PreparedStatementPtr = std::shared_ptr<sql::PreparedStatement>;
using ResultSetPtr = std::unique_ptr<sql::ResultSet>;
using SQLString = sql::SQLString;
using nlohmann::json;
using std::atomic;
using std::list;
using std::lock_guard;
using std::map;
using std::mutex;
using std::owner_less;
using std::shared_ptr;
using std::unordered_map;
using std::vector;
using std::weak_ptr;
using std::pair;
using std::queue;
using WeakWebSocketPtr = std::weak_ptr<WebSocketConnection>;