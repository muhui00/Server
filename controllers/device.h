#pragma once
#include "controllerHeaders.h"
#include <boost/signals2.hpp>

class device : public drogon::HttpController<device>
{
public:
  METHOD_LIST_BEGIN
  METHOD_ADD(device::deviceMainPageFunction, "/deviceMainPage", Get);
  METHOD_ADD(device::searchDeviceInfoByDeviceId, "/searchDeviceInfo?deviceId={1}", Get);
  METHOD_ADD(device::updateDeviceInfo, "/updateDeviceInfo", Post);
  METHOD_ADD(device::getDeviceList, "/getDeviceList?offset={1}", Get);
  METHOD_ADD(device::addNewDevicePageFunction, "/addNewDevicePage", Get);
  METHOD_ADD(device::addNewDevice, "/addNewDevice", Post);
  METHOD_LIST_END
  static std::shared_ptr<sql::Connection> conn;
  static std::mutex deviceMutex;
  static void (*modifyEstateIdOfDevice)(int deviceId, int newEstateId);
  static void (*addNewDeviceToEstate)(int deviceId, int estateId);
  void getDeviceList(const HttpRequestPtr &req, CallBackType &&callback, int offset);
  void deviceMainPageFunction(const HttpRequestPtr &req, CallBackType &&callback);
  void updateDeviceInfo(const HttpRequestPtr &req, CallBackType &&callback);
  void addNewDevicePageFunction(const HttpRequestPtr &req, CallBackType &&callback);
  void addNewDevice(const HttpRequestPtr &req, CallBackType &&callback);
  void searchDeviceInfoByDeviceId(const HttpRequestPtr &req, CallBackType &&callback, int id);
};
