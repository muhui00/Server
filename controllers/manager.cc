#include "manager.h"
#include "../macro.h"
void manager::searchManagerByNameFunction(const HttpRequestPtr &req,
                                          std::function<void(const HttpResponsePtr &)> &&callback,
                                          string managerName)
{
    if (!req->session()->find("isAdmin") or !req->session()->get<bool>("isAdmin"))
    {
        HttpResponsePtr resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("");
        callback(resp);
        return;
    }
    auto searchManagerByNameSQL = [](SqlConnPtr &conn, string managerName)
    {
        PreparedStatementPtr stmt(
            conn->prepareStatement(
                "select is_recorder,is_maintenance,is_custom_service ,is_manager,manager_password \
                    from manager where manager_name=?\
                    "));
        stmt->setString(1, managerName);
        stmt->execute();
        return stmt->getResultSet();
    };
    auto f = pool->submit(searchManagerByNameSQL, std::move(managerName));
    ResultSetPtr res = nullptr;
    res.reset(f.get());
    if (!res->next())
    {
        HttpResponsePtr resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("");
        callback(resp);
        return;
    }
    json jvalue = {
        {"asRecorder", res->getBoolean("is_recorder")},
        {"asManager", res->getBoolean("is_manager")},
        {"asMaintenance", res->getBoolean("is_maintenance")},
        {"asCustomService", res->getBoolean("is_custom_service")},
        {"managerPassword", res->getString("manager_password")}};
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setBody(nlohmann::to_string(jvalue));
    callback(resp);
}
void manager::modifyManagerPrivilegeFunction(const HttpRequestPtr &req,
                                             std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp;
    if (!req->session()->find("isAdmin") or !req->session()->get<bool>("isAdmin"))
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please login as admin first");
        callback(resp);
        return;
    }
    bool asManager = (req->getParameter("asManager") == "true");
    bool asMaintenance = (req->getParameter("asMaintenance") == "true");
    bool asCustomService = (req->getParameter("asCustomService") == "true");
    bool asRecorder = (req->getParameter("asRecorder") == "true");

    std::string managerName = req->getParameter("managerName");
    std::string managerPasswordNew = req->getParameter("newPassword");
    auto updateManagerInfoSQL = [](SqlConnPtr &conn, bool asManager, bool asMaintenance, bool asCustomService, bool asRecorder,
                                   std::string managerPasswordNew, std::string managerName)
    {
        PreparedStatementPtr stmt(
            conn->prepareStatement(
                "update manager set is_manager=?,is_maintenance=?,is_custom_service=? ,is_recorder=?,manager_password=? \
                where manager_name=?"));
        printf("manager_new_password: %s\n", managerPasswordNew.c_str());
        stmt->setBoolean(1, asManager);
        stmt->setBoolean(2, asMaintenance);
        stmt->setBoolean(3, asCustomService);
        stmt->setBoolean(4, asRecorder);
        stmt->setString(5, managerPasswordNew);
        stmt->setString(6, managerName);
        stmt->execute();
        return stmt->getResultSet();
    };
    auto f = pool->submit(updateManagerInfoSQL,
                            std::move(asManager),
                            std::move(asMaintenance),
                            std::move(asCustomService),
                            std::move(asRecorder),
                            std::move(managerPasswordNew),
                            std::move(managerName));
    f.get();
    req->session()
        ->insert(SessionCookie::MODIFY_MANAGER_PRIVILEGE_SUCCESSFULLY, true);
    resp = HttpResponse::newRedirectionResponse("/manager/modifyManagerPrivilegePage");
    callback(resp);
}
void manager::modifyManagerPrivilegePageFunction(const HttpRequestPtr &req,
                                                 std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (!req->session()->find("isAdmin") or !req->session()->get<bool>("isAdmin"))
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please login as admin first");
        callback(resp);
        return;
    }
    string fileData;
    Utils::readFile(Path::FilePath::Html::MANAGER_MODIFY_MANAGER_PAGE, fileData);
    Cookie loginCookie("userLoginOrNot", "true");
    loginCookie.setHttpOnly(false);
    resp->addCookie(loginCookie);
    Cookie adminCookie("isAdmin", "true");
    adminCookie.setHttpOnly(false);
    resp->addCookie(adminCookie);
    resp->setStatusCode(k200OK);
    resp->setBody(fileData);
    if (req->session()->find(SessionCookie::MODIFY_MANAGER_PRIVILEGE_SUCCESSFULLY))
    {
        Cookie ModifySucceedCookie(SessionCookie::MODIFY_MANAGER_PRIVILEGE_SUCCESSFULLY, "true");
        ModifySucceedCookie.setHttpOnly(false);
        resp->addCookie(ModifySucceedCookie);
        req->session()->erase(SessionCookie::MODIFY_MANAGER_PRIVILEGE_SUCCESSFULLY);
    }
    callback(resp);
}
void manager::checkNewManagerNameFunction(const HttpRequestPtr &req,
                                          std::function<void(const HttpResponsePtr &)> &&callback, string newManagerName)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find("isManager") == false)
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    if (newManagerName.size() < 4)
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    if (newManagerName.size() > 32)
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    auto getManagerInfoByManagerNameSQL = [](SqlConnPtr &conn, std::string newManagerName)
    {
        PreparedStatementPtr stmt(conn->prepareStatement(
            "select * from manager where manager_name=?"));
        stmt->setString(1, newManagerName);
        stmt->execute();
        return stmt->getResultSet();
    };
    auto f = pool->submit(getManagerInfoByManagerNameSQL, std::move(newManagerName));
    ResultSetPtr sql_res = nullptr;
    sql_res.reset(f.get());
    if (sql_res->next())
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    resp->setStatusCode(k200OK);
    resp->setBody("Legal");
    callback(resp);
    return;
}
void manager::managerMainPageFunction(const HttpRequestPtr &req,
                                      std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find("managerId") == false)
    {
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }
    string fileData;
    Utils::readFile(Path::FilePath::Html::MANAGER_MAIN_PAGE, fileData);
    auto createIdentityCookieForHtml = [&](const std::string &str)
    {
        if (req->session()->find(str) and req->session()->get<bool>(str))
        {
            resp->addCookie(Utils::createCookieForHTML(str, "true"));
        }
    };
    createIdentityCookieForHtml("isAdmin");
    createIdentityCookieForHtml("isCustomService");
    createIdentityCookieForHtml("isRecorder");
    createIdentityCookieForHtml("isManager");
    createIdentityCookieForHtml("isMaintenance");
    resp->setStatusCode(k200OK);
    resp->setBody(fileData);
    callback(resp);
}
void manager::managerLoginFunction(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp;
    string manager_name = req->getParameter("managerName");
    auto managerLoginSQL = [](SqlConnPtr &conn, std::string manager_name)
    {
        PreparedStatementPtr stmt(conn->prepareStatement(
            "select manager_password, is_administrator,manager_id,is_manager,is_custom_service,is_recorder,is_maintenance from manager where manager_name = ?;"));
        stmt->setString(1, manager_name);
        stmt->execute();
        return stmt->getResultSet();
    };
    auto f = pool->submit(managerLoginSQL, manager_name);
    ResultSetPtr sql_res = nullptr;
    sql_res.reset(f.get());
    int manager_id = -1;
    bool administor_privilege = false;
    string password = req->getParameter("managerPassword");
    if (sql_res->next())
    {
        if (
            strcmp(
                password.c_str(),
                sql_res->getString("manager_password")) == 0)
        {
            administor_privilege = sql_res->getBoolean("is_administrator");
            manager_id = sql_res->getInt("manager_id");
        }
    }
    if (manager_id == -1)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Wrong Password or Wrong Account Name!");
    }
    else
    {
        req->session()->insert("managerId", manager_id);
        req->session()->insert("isMaintenance", sql_res->getBoolean("is_maintenance"));
        req->session()->insert("isRecorder", sql_res->getBoolean("is_recorder"));
        req->session()->insert("isCustomService", sql_res->getBoolean("is_custom_service"));
        req->session()->insert("isManager", sql_res->getBoolean("is_manager"));
        if (administor_privilege)
        {
            req->session()->insert("isAdmin", true);
            resp = HttpResponse::newRedirectionResponse("/manager/managerMainPage");
        }
        else
        {
            req->session()->insert("isAdmin", false);
            resp = HttpResponse::newRedirectionResponse("/manager/managerMainPage");
        }
    }
    callback(resp);
}
void manager::createNewManagerFunction(const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp;
    if (req->session()->find("managerId") == false or req->session()->get<int>("managerId") < 0)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please login first to [Create a New Manager] !");
        callback(resp);
        return;
    }
    if (req->session()->find("isAdmin") == false or req->session()->get<bool>("isAdmin") == false)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("You have no privilege to [Create a New Manager] !");
        callback(resp);
        return;
    }

    string password = req->getParameter("newManagerPassword");
    string name = req->getParameter("newManagerName");
    if (password.size() > 32 or name.size() > 32)
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("password or name invalid");
        callback(resp);
        return;
    }
    auto _sql = [](SqlConnPtr &conn, std::string name)
    {
        PreparedStatementPtr checkNameStmt(
            conn->prepareStatement("select * from manager where manager_name=?"));
        checkNameStmt->setString(1, name);
        checkNameStmt->execute();
        return checkNameStmt->getResultSet();
    };
    auto f = pool->submit(_sql, name);

    ResultSetPtr sqlRes = nullptr;
    sqlRes.reset(f.get());
    if (sqlRes->next())
    {
        resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please change New Manager Name, As Manager Name Should Be Unique");
        callback(resp);
        return;
    }
    bool asManager = (req->getParameter("asManager") == "true");
    bool asMaintenance = (req->getParameter("asMaintenance") == "true");
    bool asCustomService = (req->getParameter("asCustomService") == "true");
    bool asRecorder = (req->getParameter("asRecorder") == "true");
    auto _sql1 = [](SqlConnPtr &conn, std::string name, string password, bool asManager, bool asMaintenance, bool asCustomService, bool asRecorder)
    {
        PreparedStatementPtr stmt(conn->prepareStatement(
            "insert into manager(is_administrator, manager_name, manager_password,is_manager,is_maintenance,is_custom_service,is_recorder) \
            values(false,?,?,?,?,?,?)"));
        stmt->setString(1, name);
        stmt->setString(2, password);
        stmt->setBoolean(3, asManager);
        stmt->setBoolean(4, asMaintenance);
        stmt->setBoolean(5, asCustomService);
        stmt->setBoolean(6, asRecorder);
        stmt->execute();
        return stmt->getResultSet();
    };
    auto _f = pool->submit(_sql1, name, password, asManager, asMaintenance, asCustomService, asRecorder);
    _f.get();
    req->session()->insert("createSuccessfully", true);
    resp = HttpResponse::newRedirectionResponse("/manager/createNewManagerPage?init=0");
    callback(resp);
}
void manager::createNewManagerPageFunction(const HttpRequestPtr &req,
                                           std::function<void(const HttpResponsePtr &)> &&callback, int init)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    string fileData;
    Utils::readFile(Path::FilePath::Html::MANAGER_CREATE_NEW_MANAGER_PAGE, fileData);
    resp->setStatusCode(k200OK);
    if (req->session()->find("isAdmin") == false)
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Unable to Create New Manager");
        callback(resp);
        return;
    }
    if (init)
    {
        resp->setBody(fileData);
        callback(resp);
        return;
    }

    if (
        req->session()->get<bool>("createSuccessfully") == false)
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Unable to Create New Manager");
        callback(resp);
        return;
    }
    if (req->session()->find("createSuccessfully") and
        req->session()->get<bool>("createSuccessfully"))
    {
        req->session()->erase("createSuccessfully");
        Cookie createCookie("createSuccessfully", "true");
        createCookie.setHttpOnly(false);
        resp->addCookie(move(createCookie));
    }
    resp->setBody(move(fileData));
    callback(resp);
}
void manager::managerLoginPageFunction(const HttpRequestPtr &req,
                                       std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    string fileData;
    Utils::readFile(Path::FilePath::Html::MANAGER_LOGIN_PAGE, fileData);
    resp->setStatusCode(k200OK);
    resp->setBody(move(fileData));
    callback(resp);
}
void manager::createNewUserPageFunction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int init)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find("isManager") == false or
        req->session()->get<bool>("isManager") == false)
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please Login As a Manager First!");
        callback(resp);
        return;
    }
    string fileData;
    Utils::readFile(Path::FilePath::Html::MANAGER_CREATE_NEW_USER_PAGE, fileData);
    resp->setStatusCode(k200OK);
    resp->setBody(fileData);
    if (init)
    {
        callback(resp);
        return;
    }
    if (req->session()->find("createSuccessfully") and
        req->session()->get<bool>("createSuccessfully"))
    {
        req->session()->erase("createSuccessfully");
        Cookie createSuccessfullyCookie("createSuccessfully", "true");
        createSuccessfullyCookie.setHttpOnly(false);
        resp->addCookie(createSuccessfullyCookie);
    }
    callback(resp);
}
void manager::manageUsersPageFunction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    string fileData;
    Utils::readFile(Path::FilePath::Html::MANAGER_CREATE_NEW_USER_PAGE, fileData);
    resp->setStatusCode(k200OK);
    resp->setBody(fileData);
    callback(resp);
}
void manager::checkNewUserNameFunction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, string newUserName)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (
        (req->session()->find("isManager") == false or req->session()->get<bool>("isManager") == false) and
        (req->session()->find("isAdmin") == false or req->session()->get<bool>("isAdmin") == false))
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    if (newUserName.size() < 4)
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    if (newUserName.size() > 32)
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    auto f = pool->submit([](SqlConnPtr &conn, string newUserName)
                            {
        PreparedStatementPtr stmt(conn->prepareStatement(
        "select * from user where user_name=?"));
        stmt->setString(1, newUserName);
        stmt->execute();
        return stmt->getResultSet(); },
                            newUserName);

    ResultSetPtr sql_res = nullptr;
    sql_res.reset(f.get());
    if (sql_res->next())
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    resp->setStatusCode(k200OK);
    resp->setBody("Legal");
    callback(resp);
    return;
}
void manager::createNewUserFunction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (
        req->session()->find("isManager") == false or
        req->session()->get<bool>("isManager") == false)
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("You are not allowed to create a new user");
        callback(resp);
        return;
    }
    string newUserName = req->getParameter("newUserName");
    string newUserPassword = req->getParameter("newUserPassword");
    if (newUserName.size() > 32 or newUserPassword.size() > 32)
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Too long password or userName, please reset.");
        callback(resp);
        return;
    }
    auto f = pool->submit(
        [](SqlConnPtr &conn, string newUserName)
        {
            PreparedStatementPtr stmt(
                conn->prepareStatement(
                    "select * from user where user_name=?"));
            stmt->setString(1, newUserName);
            stmt->execute();
            return stmt->getResultSet();
        },
        newUserName);
    ResultSetPtr sql_res = nullptr;
    sql_res.reset(f.get());
    if (sql_res->next())
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("User Name Exists!");
        callback(resp);
        return;
    }
    auto f1 = pool->submit(
        [](SqlConnPtr &conn, string newUserName, string newUserPassword)
        {
            PreparedStatementPtr _stmt(conn->prepareStatement(
                "insert into user(is_temporal, expire_time , user_name ,password) values(?,?,?,?)"));
            _stmt->setBoolean(1, false);
            _stmt->setNull(2, sql::Types::TIME);
            _stmt->setString(3, newUserName);
            _stmt->setString(4, newUserPassword);
            _stmt->execute();
        },
        std::move(newUserName), std::move(newUserPassword));
    f1.get();
    req->session()->insert("createSuccessfully", true);
    resp = HttpResponse::newRedirectionResponse("/manager/createNewUserPage?init=0");
    callback(resp);
}
