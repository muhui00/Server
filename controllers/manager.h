#pragma once

#include "controllerHeaders.h"
class manager : public drogon::HttpController<manager>
{
public:
  static DBThreadPoolPtr pool;
  METHOD_LIST_BEGIN
  METHOD_ADD(manager::createNewUserPageFunction, "/createNewUserPage?init={1}", Get);
  METHOD_ADD(manager::createNewUserFunction, "/createNewUser", Post);

  METHOD_ADD(manager::managerLoginPageFunction, "/managerLoginPage", Get);
  METHOD_ADD(manager::managerLoginFunction, "/managerLogin", Post);

  METHOD_ADD(manager::modifyManagerPrivilegePageFunction, "/modifyManagerPrivilegePage", Get);
  METHOD_ADD(manager::modifyManagerPrivilegeFunction, "/modifyManagerPrivilege", Post);
  METHOD_ADD(manager::searchManagerByNameFunction, "/searchManagerByName?managerName={1}", Get);

  METHOD_ADD(manager::createNewManagerPageFunction, "/createNewManagerPage?init={1}", Get, Post);
  METHOD_ADD(manager::createNewManagerFunction, "/createNewManager", Post);
  METHOD_ADD(manager::checkNewManagerNameFunction, "/checkNewManagerName?newManagerName={1}", Get);

  METHOD_ADD(manager::managerMainPageFunction, "/managerMainPage", Get);
  METHOD_ADD(manager::manageUsersPageFunction, "/manageUsersPage", Get);
  METHOD_ADD(manager::checkNewUserNameFunction, "/checkNewUserName?newUserName={1}", Get);
  METHOD_LIST_END
  void searchManagerByNameFunction(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback,
                                   string managerName);
  void modifyManagerPrivilegeFunction(const HttpRequestPtr &req,
                                      std::function<void(const HttpResponsePtr &)> &&callback);
  void modifyManagerPrivilegePageFunction(const HttpRequestPtr &req,
                                          std::function<void(const HttpResponsePtr &)> &&callback);
  void managerMainPageFunction(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback);
  void managerLoginFunction(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback);
  void checkNewManagerNameFunction(const HttpRequestPtr &req,
                                   std::function<void(const HttpResponsePtr &)> &&callback, string newManagerName);
  void createNewManagerFunction(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback);
  void createNewManagerPageFunction(const HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&callback, int init);
  void managerLoginPageFunction(const HttpRequestPtr &req,
                                std::function<void(const HttpResponsePtr &)> &&callback);
  void manageUsersPageFunction(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback);
  void createNewUserPageFunction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int init);
  void createNewUserFunction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
  void checkNewUserNameFunction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, string newUserName);
};