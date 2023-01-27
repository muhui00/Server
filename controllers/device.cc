#include "device.h"
std::mutex device::deviceMutex;
void device::getDeviceList(const HttpRequestPtr &req, CallBackType &&callback, int offset)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find(SessionCookie::IS_MAINTENANCE) == false or req->session()->get<bool>(SessionCookie::IS_MAINTENANCE) == false)
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please Login As Maintenance Stuff");
        callback(resp);
        return;
    }
    printf("[DB-test-getNumberOfDevices-PRE]:\n");
    auto sql = [](SqlConnPtr &conn)
    {
        PreparedStatementPtr stmt(conn->prepareStatement(
            "select count(1) as number_devices from device"));
        stmt->execute();
        return stmt->getResultSet();
    };
    auto f = pool->submit(sql);
    printf("[DB-test-getNumberOfDevices]\n");
    ResultSetPtr fetchNumberDevicesSQLResult(f.get());
    /* ResultSetPtr fetchNumberDevicesSQLResult=nullptr;
    fetchNumberDevicesSQLResult.reset(f.get()); */
    fetchNumberDevicesSQLResult->next();
    int numberDevices = fetchNumberDevicesSQLResult->getInt("number_devices");
    printf("[DB-test-getNumberOfDevices]:%d\n", numberDevices);
    auto f1 = pool->submit(
        [](SqlConnPtr &conn, int offset)
        {
            PreparedStatementPtr stmt(conn->prepareStatement(
                "select device_id,state,estate_id,device_alias \
                from device \
                order by device_id  \
                limit ?,?"));
            stmt->setInt(1, offset * DEVICE_INFO_LIST_NUMBER_ITEM);
            stmt->setInt(2, DEVICE_INFO_LIST_NUMBER_ITEM);
            stmt->execute();
            return stmt->getResultSet();
        },
        offset);
    ResultSetPtr res(f1.get());
    json deviceInfoListJson = {};
    while (res->next())
    {
        deviceInfoListJson.push_back(
            (json){
                {"deviceId", res->getInt("device_id")},
                {"state", res->getString("state")},
                {"estateId", res->getInt("estate_id")},
                {"deviceAlias", res->getString("device_alias")}});
    }
    resp->setStatusCode(k200OK);
    resp->setBody(
        (json){
            {"deviceInfoList", deviceInfoListJson},
            {"curListPage", offset},
            {"numberOfDevice", numberDevices}}
            .dump());
    callback(resp);
}
void device::deviceMainPageFunction(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find(SessionCookie::IS_MAINTENANCE) == false or req->session()->get<bool>(SessionCookie::IS_MAINTENANCE) == false)
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please Login As Maintenance Stuff");
        callback(resp);
        return;
    }
    resp->setStatusCode(k200OK);
    string fileData = "";
    Utils::readFile(Path::FilePath::Html::DEVICE_INFO_MAIN_PAGE, fileData);
    resp->setBody(fileData);
    resp->addCookie(Utils::createCookieForHTML(SessionCookie::IS_MAINTENANCE, "true"));
    if (req->session()->find(SessionCookie::MODIFY_DEVICE_INFO_SUCCESSFULLY))
    {
        req->session()->erase(SessionCookie::MODIFY_DEVICE_INFO_SUCCESSFULLY);
        resp->addCookie(Utils::createCookieForHTML(SessionCookie::MODIFY_DEVICE_INFO_SUCCESSFULLY, "true"));
    }
    callback(resp);
}
void device::updateDeviceInfo(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find(SessionCookie::IS_MAINTENANCE) == false or req->session()->get<bool>(SessionCookie::IS_MAINTENANCE) == false)
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please Login As Maintenance Stuff");
        callback(resp);
        return;
    }

    string rawDeviceId = req->getParameter("deviceId");
    string rawDeviceState = req->getParameter("deviceState");
    string rawEstateIdOfCurDevice = req->getParameter("estateId");
    string rawDeviceAlias = req->getParameter("deviceAlias");
    int deviceId = atoi(rawDeviceId.c_str());
    int estateId = atoi(rawEstateIdOfCurDevice.c_str());
    printf("%s,%s,%s,%s\n", rawDeviceId.c_str(), rawDeviceState.c_str(), rawEstateIdOfCurDevice.c_str(), rawDeviceAlias.c_str());
    auto f = pool->submit(
        [](SqlConnPtr &conn, int deviceId)
        {
            PreparedStatementPtr stmt(conn->prepareStatement(
                "select 1 from device where device_id=?"));
            stmt->setInt(1, deviceId);
            stmt->execute();
            return stmt->getResultSet();
        },
        deviceId);

    ResultSetPtr res(f.get());
    if (!res->next())
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    auto f1 = pool->submit(
        [](SqlConnPtr &conn, int estateId)
        {
            auto stmt = conn->prepareStatement(
                "select 1 from estate where estate_id=?");
            stmt->setInt(1, estateId);
            stmt->execute();
            auto res = stmt->getResultSet();
            return res;
        },
        estateId);

    ResultSetPtr res1(f1.get());
    if (res1->next() == false)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    auto f2 = pool->submit(
        [](SqlConnPtr &conn, string rawDeviceAlias, int estateId, string rawDeviceState, int deviceId)
        {
            auto stmt = conn->prepareStatement(
                "update device set device_alias=?, estate_id=?,state=? where device_id=?");
            stmt->setString(1, rawDeviceAlias);
            stmt->setInt(2, estateId);
            stmt->setString(3, rawDeviceState);
            stmt->setInt(4, deviceId);
            stmt->execute();
            return stmt->getResultSet();
        },
        std::move(rawDeviceAlias), std::move(estateId), std::move(rawDeviceState), std::move(deviceId));
    f2.get();
    resp = HttpResponse::newRedirectionResponse("/device/deviceMainPage");
    req->session()->insert(SessionCookie::MODIFY_DEVICE_INFO_SUCCESSFULLY, true);
    modifyEstateIdOfDevice(deviceId, estateId);
    callback(resp);
}
void device::addNewDevicePageFunction(const HttpRequestPtr &req, CallBackType &&callback)
{
    if (req->session()->find(SessionCookie::IS_MAINTENANCE) == false or req->session()->get<bool>(SessionCookie::IS_MAINTENANCE) == false)
    {
        HttpResponsePtr resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please Login As Maintenance Stuff");
        callback(resp);
        return;
    }
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    string fileData = "";
    Utils::readFile(Path::FilePath::Html::DEVICE_ADD_NEW_PAGE, fileData);
    resp->setBody(fileData);
    if (req->session()->find(SessionCookie::ADD_NEW_DEVICE_SUCCESSFULLY) and req->session()->get<bool>(SessionCookie::ADD_NEW_DEVICE_SUCCESSFULLY) == true)
    {
        resp->addCookie(Utils::createCookieForHTML(SessionCookie::ADD_NEW_DEVICE_SUCCESSFULLY, "true"));
        resp->addCookie(Utils::createCookieForHTML(SessionCookie::NEW_DEVICE_ID, std::to_string(req->session()->get<int>(SessionCookie::NEW_DEVICE_ID))));
        req->session()->erase(SessionCookie::NEW_DEVICE_ID);
        req->session()->erase(SessionCookie::ADD_NEW_DEVICE_SUCCESSFULLY);
    }
    callback(resp);
}
void device::addNewDevice(const HttpRequestPtr &req, CallBackType &&callback)
{
    if (req->session()->find(SessionCookie::IS_MAINTENANCE) == false or req->session()->get<bool>(SessionCookie::IS_MAINTENANCE) == false)
    {
        return;
    }
    string rawEstateId = req->getParameter("estateId");
    int estateId = atoi(rawEstateId.c_str());
    string rawDeviceAlias = req->getParameter("deviceAlias");
    string rawDeviceState = req->getParameter("deviceState");
    printf("get args: %s, %s, %s\n", rawEstateId.c_str(), rawDeviceAlias.c_str(), rawDeviceState.c_str());

    // std::mutex for new device id;
    std::lock_guard<std::mutex> lg{deviceMutex};
    auto f = pool->submit(
        [](SqlConnPtr &conn, int estateId, string rawDeviceState, string rawDeviceAlias)
        {
            auto stmt = conn->prepareStatement(
                " INSERT INTO device(estate_id,state,device_alias) VALUES(?,?,?);");
            stmt->setInt(1, estateId);
            stmt->setString(2, rawDeviceState);
            stmt->setString(3, rawDeviceAlias);
            stmt->execute();
        },
        estateId, rawDeviceState, rawDeviceAlias);
    f.get();
    auto f1 = pool->submit(
        [](SqlConnPtr &conn)
        {
            auto stmt = conn->prepareStatement("select max(device_id) as max_device_id from device");
            stmt->execute();
            return stmt->getResultSet();
        });
    ResultSetPtr res(f1.get());
    lg.~lock_guard();
    res->next();
    int newDeviceId = res->getInt("max_device_id");
    req->session()->insert(SessionCookie::ADD_NEW_DEVICE_SUCCESSFULLY, true);
    req->session()->insert(SessionCookie::NEW_DEVICE_ID, newDeviceId);
    HttpResponsePtr resp = HttpResponse::newRedirectionResponse("/device/addNewDevicePage");
    addNewDeviceToEstate(newDeviceId, atoi(rawEstateId.c_str()));
    callback(resp);
    return;
}
void device::searchDeviceInfoByDeviceId(const HttpRequestPtr &req, CallBackType &&callback, int deviceId)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find(SessionCookie::IS_MAINTENANCE) == false or req->session()->get<bool>(SessionCookie::IS_MAINTENANCE) == false)
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please Login As Maintenance Stuff");
        callback(resp);
        return;
    }
    auto f = pool->submit(
        [](SqlConnPtr &conn, int id)
        {
            auto stmt = conn->prepareStatement("select device_id, device_alias, estate_id, state from device where device_id=?");
            stmt->setInt(1, id);
            stmt->execute();
            return stmt->getResultSet();
        },
        deviceId);

    ResultSetPtr res(f.get());
    if (!res->next())
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Device Id Not Found");
        callback(resp);
        return;
    }
    resp->setStatusCode(k200OK);
    resp->setBody(
        (json){
            {"deviceId", res->getInt("device_id")},
            {"deviceAlias", res->getString("device_alias")},
            {"estateId", res->getInt("estate_id")},
            {"state", res->getString("state")}}
            .dump());

    callback(resp);
    return;
}
// Add definition of your processing function here
