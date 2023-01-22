#pragma once

#include <nlohmann/json.hpp>
#include <list>
#include "controllerHeaders.h"

class ReceiverInfo
{
public:
  int deviceId;
  int estateIdSelected;
  SessionPtr sessionPtr;
  atomic<int> customServiceRequestState;
  int connectionId;
  int requestId;
  WeakWebSocketPtr senderPtr;
  WeakWebSocketPtr customServicePtr;
  WeakWebSocketPtr receiverPtr;
  mutex itemMutex;
  ReceiverInfo()
  {
    requestId = -1;
    connectionId = -1;
    customServiceRequestState = CustomServiceStates::USER_NO_DEVICE_CONNECTION;
  }
  ReceiverInfo(ReceiverInfo &&){};
};
class DeviceInfo
{
public:
  int estateId;
  int deviceId;
  int state;
  int connectionId;
  WeakWebSocketPtr senderPtr;
  weak_ptr<ReceiverInfo> receiverInfoPtr;
  mutex itemMutex;
  DeviceInfo()
  {
    state = DeviceVideoConnectionStates::DISCONNECTED;
  }
  DeviceInfo(DeviceInfo &&){};
};

class CustomServiceStuffInfo
{
public:
  int customServiceState;
  int requestId;
  weak_ptr<ReceiverInfo> receiverInfoPtr;
  CustomServiceStuffInfo(int requestId = -1)
  {
    this->requestId = requestId;
    customServiceState = CustomServiceStates::CUSTOM_SERVICE_FREE;
  }
};
class RequestDescription
{
public:
  int requestId;
  RequestDescription() {}
};
#define MAX_NUM_DEVICE 1024u
class websocket : public drogon::WebSocketController<websocket>
{
public:
  static vector<DeviceInfo> deviceInfoArray;
  static unordered_map<int, vector<vector<DeviceInfo>::iterator>> estateId2DeviceArray;
  static map<int, vector<DeviceInfo>::iterator> deviceId2DeviceInfoIter;
  static void modifyEstateIdOfDevice(int deviceId, int newEstateId);
  static void addNewDevice(int deviceId, int estateId);
  map<WeakWebSocketPtr, shared_ptr<ReceiverInfo>, owner_less<WeakWebSocketPtr>> receiverPtr2receiverInfo;
  map<WeakWebSocketPtr, vector<DeviceInfo>::iterator, owner_less<WeakWebSocketPtr>>
      senderPtr2DeviceInfoIter;
  map<WeakWebSocketPtr, CustomServiceStuffInfo, owner_less<WeakWebSocketPtr>> customServicePtr2ServiceInfo;
  static mutex estateMutex;
  static mutex deviceArrayMutex;
  static mutex deviceIdMutex;
  mutex requestMutex;
  mutex customServiceMutex;
  mutex receiverMutex;
  mutex senderMutex;
  atomic<int> requestId{0};
  atomic<int> connectionId{0};
  vector<pair<weak_ptr<ReceiverInfo>, RequestDescription>> requestQueue;
  int requestEmplaceBack(shared_ptr<ReceiverInfo> info);
  bool deleteRequestById(int requestId);
  json getRequestList();
  pair<weak_ptr<ReceiverInfo>, RequestDescription> getRequestInfoById(int requestId);
  json getDeviceInfoByEstateId(int estateId);
  void handleNewMessageFromSender(json &jmsg, std::string &&message, WebSocketConnectionPtr wsConnPtr);
  void handleNewMessageFromReceiver(json &jmsg, std::string &&message, WebSocketConnectionPtr wsConnPtr);
  void handleNewMessageFromCustomService(json &jmsg, std::string &&message, WebSocketConnectionPtr wsConnPtr);
  void handleNewMessage(const WebSocketConnectionPtr &,
                        std::string &&,
                        const WebSocketMessageType &) override;
  void handleNewConnection(const HttpRequestPtr &,
                           const WebSocketConnectionPtr &) override;
  void handleConnectionClosed(const WebSocketConnectionPtr &) override;
  WS_PATH_LIST_BEGIN
  WS_PATH_ADD(Path::WebSocketPath::RECEIVER_CONNECT_PATH);
  WS_PATH_ADD(Path::WebSocketPath::SENDER_CONNECT_PATH);
  WS_PATH_ADD(Path::WebSocketPath::CUSTOM_SERVICE_CONNECT_PATH);
  WS_PATH_LIST_END
};