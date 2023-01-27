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
shared_ptr<DataBaseThreadPool> dbPool = nullptr;
shared_ptr<DataBaseThreadPool> manager::pool = nullptr;
shared_ptr<DataBaseThreadPool> estate::pool = nullptr;
shared_ptr<DataBaseThreadPool> user::pool = nullptr;
shared_ptr<DataBaseThreadPool> device::pool = nullptr;

void databaseInitializer()
{
    dbPool = std::make_shared<DataBaseThreadPool>(4, 128, "test_dba", "password", "jdbc:mariadb://localhost:3306/test");
    manager::pool = dbPool;
    estate::pool = dbPool;
    user::pool = dbPool;
    device::pool = dbPool;
}
void systemInitializer()
{
    databaseInitializer();
    auto getEstateData = [](SqlConnPtr &conn)
    {
        PreparedStatementPtr stmt(
            conn->prepareStatement(
                "select estate_id from estate where is_active=true"));
        stmt->execute();
        return stmt->getResultSet();
    };

    std::future<sql::ResultSet *> ret = dbPool->submit(getEstateData);
    std::unique_ptr<sql::ResultSet> sqlRes = nullptr;
    sqlRes.reset(ret.get());
    while (sqlRes->next())
    {
        websocket::estateId2DeviceArray[sqlRes->getInt("estate_id")];
    }
    // in case of the lose of validation of vector::iterator
    websocket::deviceInfoArray.reserve(MAX_NUM_DEVICE);
    auto getDeviceInfo = [](SqlConnPtr &conn)
    {
        PreparedStatementPtr stmt = nullptr;
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
        return stmt->getResultSet();
    };
    ret = dbPool->submit(getDeviceInfo);
    sqlRes.reset(ret.get());
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
        printf("[Test-DB--GetDeviceId--]:%d\n", curDeviceId);
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
                             CallBackType &&callback)
                          {
                              std::string defaultHtmlFile = "";
                              Utils::readFile(Path::FilePath::Html::DEFAULT_MAIN_PAGE, defaultHtmlFile);
                              HttpResponsePtr resp = HttpResponse::newHttpResponse();
                              resp->setBody(defaultHtmlFile);
                              resp->setStatusCode(k200OK);
                              callback(resp);
                          });
    app().run();
    return 0;
}
