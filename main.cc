#include <drogon/drogon.h>
#include <mariadb/conncpp.hpp>
#include "utils.h"
#include "macro.h"
#include "controllers/user.h"
#include "controllers/manager.h"
#include "controllers/websocket.h"
#include "controllers/estate.h"
#include "controllers/device.h"
using namespace drogon;
using namespace std;
using PreparedStatementPtr = std::shared_ptr<sql::PreparedStatement>;
using ResultSetPtr = std::unique_ptr<sql::ResultSet>;
using SQLString = sql::SQLString;

void (*device::modifyEstateIdOfDevice)(int deviceId, int newEstateId) = websocket::modifyEstateIdOfDevice;
void (*device::addNewDeviceToEstate)(int deviceId, int estateId) = websocket::addNewDevice;
std::shared_ptr<sql::Connection> conn;
shared_ptr<sql::Connection> manager::conn = nullptr;
shared_ptr<sql::Connection> user::conn = nullptr;
shared_ptr<sql::Connection> estate::conn = nullptr;
shared_ptr<sql::Connection> device::conn = nullptr;

void databaseInitializer()
{
    sql::Driver *driver = sql::mariadb::get_driver_instance();
    sql::SQLString url("jdbc:mariadb://localhost:3306/test");
    sql::Properties properties({{"user", "test_dba"},
                                {"password", "password"}});
    conn.reset(driver->connect(url, properties));
    manager::conn = conn;
    user::conn = conn;
    estate::conn = conn;
    device::conn = conn;
    if (!conn)
    {
        cerr << "!!!Invalid Database Connection!!!" << endl;
        exit(EXIT_FAILURE);
    }
}
void systemInitializer()
{
    databaseInitializer();
    PreparedStatementPtr stmt(
        conn->prepareStatement(
            "select estate_id from estate where is_active=true"));
    stmt->execute();
    ResultSetPtr sqlRes(
        stmt->getResultSet());
    while (sqlRes->next())
    {
        websocket::estateId2DeviceArray[sqlRes->getInt("estate_id")];
    }
    // in case of the lose of validation of vector::iterator
    websocket::deviceInfoArray.reserve(MAX_NUM_DEVICE);
    stmt.reset();
    stmt.reset(conn->prepareStatement(
        "                                                                                   \
        select device_id, estate_id                                                         \
        from device d where exists                                                          \
        (                                                                                   \
            select estate_id from estate where is_active = true and estate_id=d.estate_id   \
        )                                                                                   \
        order by estate_id,  device_id                                                      \
        "));
    stmt->execute();
    sqlRes.reset(
        stmt->getResultSet());
    int pre = -1;
    int curIndex = 0;
    int cnt = 0;
    while (sqlRes->next())
    {
        int curEstateId = sqlRes->getInt("estate_id");
        int curDeviceId = sqlRes->getInt("device_id");
        if (pre == curEstateId)
        {
            curIndex += 1;
        }
        else
        {
            pre = curEstateId;
            curIndex = 0;
        }
        cnt += 1;
        websocket::deviceInfoArray.emplace_back();
        websocket::deviceInfoArray.back().estateId = curEstateId;
        websocket::deviceInfoArray.back().deviceId = curDeviceId;
        websocket::deviceId2DeviceInfoIter[curDeviceId] = std::prev(websocket::deviceInfoArray.end());
        websocket::estateId2DeviceArray[curEstateId].push_back(std::prev(websocket::deviceInfoArray.end()));
    }
}
int main()
{
    systemInitializer();
    app().enableSession();
    app().addListener("0.0.0.0", 1234);
    app().registerHandler("/",
                          [](const HttpRequestPtr &req,
                             CallBackType &&callback) {
                                std::string defaultHtmlFile="";
                                Utils::readFile(Path::FilePath::Html::DEFAULT_MAIN_PAGE,defaultHtmlFile);
                                HttpResponsePtr resp=HttpResponse::newHttpResponse();
                                resp->setBody(defaultHtmlFile);
                                resp->setStatusCode(k200OK);
                                callback(resp);
                          });
    app().run();
    return 0;
}
