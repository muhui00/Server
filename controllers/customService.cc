#include "customService.h"

void customService::servicePageFunction(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find("isCustomService") == false or req->session()->get<bool>("isCustomService") == false)
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please Login As Custom Service");
        callback(resp);
        return;
    }
    resp->addCookie(Utils::createCookieForHTML("isCustomService", "true"));
    string fileData = "";
    resp->setStatusCode(k200OK);
    Utils::readFile(Path::FilePath::Html::CUSTOM_SERVICE_MAIN_PAGE, fileData);
    resp->setBody(fileData);
    callback(resp);
}
void customService::customServicePeerConnectionScript(const HttpRequestPtr &req, CallBackType &&callback)
{
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    if (req->session()->find("isCustomService") == false or req->session()->get<bool>("isCustomService") == false)
    {
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Please Login As Custom Service");
        callback(resp);
        return;
    }
    string fileData = "";
    Utils::readFile(Path::FilePath::Javascript::CUSTOM_SERVICE_PEER_CONNECTION, fileData);
    resp->setStatusCode(k200OK);
    resp->setBody(fileData);
    callback(resp);
}