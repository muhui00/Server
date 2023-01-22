#include "user.h"

void user::peerConnectionScript(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    std::string fileData = "";
    Utils::readFile(Path::FilePath::Javascript::USER_PEER_CONNECTION, fileData);
    resp->setStatusCode(k200OK);
    resp->setBody(fileData);
    callback(resp);
}
void user::selectEstateFunction(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback,
                                int estate_id)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find("isLogin") == false ||
        req->session()->get<bool>("isLogin") == false)
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
    }

    PreparedStatementPtr stmt(
        conn->prepareStatement(
            "select estate_name from estate where estate_id = ? and is_active = true"));
    stmt->setInt(1, estate_id);
    stmt->execute();
    ResultSetPtr res(stmt->getResultSet());
    if (!res->next())
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid Request");
        callback(resp);
        return;
    }
    resp->setStatusCode(k200OK);
    string fileData;
    Utils::readFile(Path::FilePath::Html:: USER_VIDEO_DISPLAY_PAGE, fileData);
    resp->setBody(fileData);
    Cookie estateIdCookie("estateId", to_string(estate_id));
    estateIdCookie.setHttpOnly(false);
    resp->addCookie(move(estateIdCookie));
    callback(resp);
}
void user::selectEstatePageFunction(const HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find("isLogin") == false or !req->session()->get<bool>("isLogin"))
    {
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Please login first");
        callback(resp);
        return;
    }
    string fileData;
    Utils::readFile(Path::FilePath::Html:: USER_SELECT_ESTATE_PAGE, fileData);
    PreparedStatementPtr stmt(
        conn->prepareStatement(
            "select estate_id,estate_name,estate_description from estate where is_active=true"));
    stmt->execute();
    ResultSetPtr sqlRes(
        stmt->getResultSet());
    nlohmann::json jvalue;
    while (sqlRes->next())
    {
        nlohmann::json curItem = {{"estate_id", sqlRes->getInt("estate_id")},
                                  {"estate_name", sqlRes->getString("estate_name")},
                                  {"estate_description", sqlRes->getString("estate_description")}};
        jvalue.push_back(curItem);
    }
    string tagAndJvalue = "<script>\nvar estate_info_list=" + to_string(jvalue) + "\n</script>";
    resp->setStatusCode(k200OK);
    resp->setBody(fileData + tagAndJvalue);
    callback(resp);
}
void user::userLoginPageFunction(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&callback)
{

    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    string fileData;
    Utils::readFile(Path::FilePath::Html:: USER_LOGIN_PAGE, fileData);
    resp->setStatusCode(k200OK);
    resp->setBody(fileData);
    callback(resp);
}
void user::userLoginFunction(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    // check user passwords
    PreparedStatementPtr getUserPasswordStatement(conn->prepareStatement(
        "select password,user_id from user where user_name=?"));
    SQLString loginUserName = req->getParameter("userName");
    getUserPasswordStatement->setString(1, loginUserName);
    getUserPasswordStatement->execute();
    ResultSetPtr sql_res(getUserPasswordStatement->getResultSet());
    bool loginSuccessful = false;
    cout << req->getParameter("userName") << endl;
    if (sql_res->next())
    {
        if (!strcmp(sql_res->getString("password").c_str(),
                    req->getParameter("userPassword").c_str()))
        {
            loginSuccessful = 1;
            req->session()->insert("userId", sql_res->getInt("user_id"));
            Cookie userLoginCookie("userLoginOrNot", "true");
            Cookie userNameCookie("userName", loginUserName.c_str());
            userLoginCookie.setHttpOnly(false);
            userNameCookie.setHttpOnly(false);
            resp->addCookie(userLoginCookie);
            resp->addCookie(userNameCookie);
        }
    }
    if (loginSuccessful)
    {
        req->session()->insert("isLogin", true);
        resp = HttpResponse::newRedirectionResponse("/user/userMainPage");
        callback(resp);
    }
    else
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("User name not exists or wrong password!");
    }
}

void user::userMainPageFunction(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    string fileData;
    if (req->session()->find("isLogin") == false or req->session()->get<bool>("isLogin") == false)
    {
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    Utils::readFile(Path::FilePath::Html:: USER_MAIN_PAGE, fileData);
    resp->setBody(fileData);
    resp->setStatusCode(k200OK);
    Cookie userLoginCookie("userLoginOrNot", "true");
    userLoginCookie.setHttpOnly(false);
    resp->addCookie(userLoginCookie);
    callback(resp);
}
