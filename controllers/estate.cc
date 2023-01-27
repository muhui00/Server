#include "estate.h"

void estate::createNewEstateInfo(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp;
    if (
        (req->session()->find("isManager") == false or req->session()->get<bool>("isManager") == false) and
        (req->session()->find("isAdmin") == false or req->session()->get<bool>("isAdmin") == false))
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please login as manager or as administrator first!");
        callback(resp);
        return;
    }
    string estateName = req->getParameter("estateName");
    string estateDescription = req->getParameter("estateDescription");
    if (estateName.size() <= 3 or estateName.size() > 32 or estateDescription.size() > 256)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid input");
        callback(resp);
        return;
    }
    auto createNewEstateDB = [&](SqlConnPtr &conn, string estateName, string estateDescription)
    {
        PreparedStatementPtr statement(
            conn->prepareStatement(
                "insert into estate(estate_name,estate_description,is_active) values(?,?,true)"));
        statement->setString(1, estateName);
        statement->setString(2, estateDescription);
        statement->execute();
        return statement->getResultSet();
    };
    auto ret = pool->submit(createNewEstateDB, std::move(estateName), std::move(estateDescription));
    ret.get();
    resp = HttpResponse::newRedirectionResponse("/estate/createNewEstateInfoPage");
    callback(resp);
}
void estate::createNewEstateInfoPage(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp;
    if (
        (req->session()->find("isManager") == false or req->session()->get<bool>("isManager") == false) and
        (req->session()->find("isAdmin") == false or req->session()->get<bool>("isAdmin") == false))
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please login as manager or as administrator first!");
        callback(resp);
        return;
    }

    resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    {
        Cookie managerCookie("isManager", "true");
        Cookie adminCookie("isAdmin", "true");
        managerCookie.setHttpOnly(false);
        adminCookie.setHttpOnly(false);
        resp->addCookie(managerCookie);
        resp->addCookie(adminCookie);
        if (req->session()->find(SessionCookie::CREATE_NEW_ESTATE_SUCCESSFULLY) and req->session()->get<bool>(SessionCookie::CREATE_NEW_ESTATE_SUCCESSFULLY))
        {
            req->session()->erase(SessionCookie::CREATE_NEW_ESTATE_SUCCESSFULLY);
            Cookie createMsgCookie(SessionCookie::CREATE_NEW_ESTATE_SUCCESSFULLY, "true");
            createMsgCookie.setHttpOnly(false);
            resp->addCookie(createMsgCookie);
        }
    }
    string fileData;
    Utils::readFile(Path::FilePath::Html::CREATE_NEW_ESTATE_INFO_PAGE, fileData);
    resp->setBody(fileData);
    callback(resp);
}
void estate::modifyEstateInfo(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp;
    if (
        (req->session()->find("isManager") == false or req->session()->get<bool>("isManager") == false) and
        (req->session()->find("isAdmin") == false or req->session()->get<bool>("isAdmin") == false))
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please login as manager or as administrator first!");
        callback(resp);
        return;
    }

    string estateName = req->getParameter("estateName");
    string estateDescription = req->getParameter("estateDescription");
    bool isActive = (req->getParameter("isActive") == "true");
    int estateId = atoi(req->getParameter("estateId").c_str());
    if (estateName.size() <= 3 or estateName.size() > 32 or estateDescription.size() > 256)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid input");
        callback(resp);
        return;
    }

    auto updateEstateInfoSQL = [](SqlConnPtr &conn, string estateName, string estateDescription, bool isActive, int estateId)
    {
        PreparedStatementPtr statement(
            conn->prepareStatement(
                "update estate set estate_name=?,estate_description=?,is_active=? where estate_id=?"));
        statement->setString(1, estateName);
        statement->setString(2, estateDescription);
        statement->setBoolean(3, isActive);
        statement->setInt(4, estateId);
        statement->execute();
        return statement->getResultSet();
    };
    auto f = pool->submit(updateEstateInfoSQL, std::move(estateName), std::move(estateDescription), std::move(isActive), std::move(estateId));
    f.get();
    req->session()->insert(SessionCookie::MODIFY_ESTATE_SUCCESSFULLY, true);
    resp = HttpResponse::newRedirectionResponse("/estate/estateListPage");
    callback(resp);
}
void estate::searchEstateFunction(const HttpRequestPtr &req, CallBackType &&callback, int offset, int num)
{
    HttpResponsePtr resp;
    if (req->session()->find(SessionCookie::MANAGER_ID) == false)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please login as manager or as administrator first!");
        callback(resp);
        return;
    }
    auto updateEstateInfoSQL = [](SqlConnPtr &conn, int offset, int num)
    {
        PreparedStatementPtr stmt(
            conn->prepareStatement(
                "select estate_id,estate_name, estate_description, is_active from estate where estate_id>? \
            order by estate_id ASC limit ? "));
        stmt->setInt(1, offset);
        stmt->setInt(2, num);
        stmt->execute();
        return stmt->getResultSet();
    };
    auto f = pool->submit(updateEstateInfoSQL, std::move(offset), std::move(num));
    std::unique_ptr<sql::ResultSet> sqlRes = nullptr;
    sqlRes.reset(f.get());
    nlohmann::json jvalue = {};
    while (sqlRes->next())
    {
        jvalue.push_back({{"estate_id", sqlRes->getInt("estate_id")},
                          {"estate_name", sqlRes->getString("estate_name")},
                          {"estate_description", sqlRes->getString("estate_description")},
                          {"is_active", sqlRes->getBoolean("is_active")}});
    }
    resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setBody(to_string(jvalue));
    callback(resp);
}

void estate::estateListPageFunction(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp;
    if (req->session()->find(SessionCookie::MANAGER_ID) == false)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please login as manager or as administrator first!");
        callback(resp);
        return;
    }

    resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    {
        Cookie managerCookie("isManager", "true");
        Cookie adminCookie("isAdmin", "true");
        managerCookie.setHttpOnly(false);
        adminCookie.setHttpOnly(false);
        resp->addCookie(managerCookie);
        resp->addCookie(adminCookie);
        if (req->session()->find(SessionCookie::MODIFY_ESTATE_SUCCESSFULLY) and req->session()->get<bool>(SessionCookie::MODIFY_ESTATE_SUCCESSFULLY))
        {
            req->session()->erase(SessionCookie::MODIFY_ESTATE_SUCCESSFULLY);
            Cookie modifyMsgCookie(SessionCookie::MODIFY_ESTATE_SUCCESSFULLY, "true");
            modifyMsgCookie.setHttpOnly(false);
            resp->addCookie(modifyMsgCookie);
        }
    }
    string fileData;
    Utils::readFile(Path::FilePath::Html::ESTATE_LIST_PAGE, fileData);
    resp->setBody(fileData);
    callback(resp);
}
