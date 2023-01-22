#include "websocket.h"

vector<DeviceInfo> websocket::deviceInfoArray;
unordered_map<int, vector<vector<DeviceInfo>::iterator>> websocket::estateId2DeviceArray;
map<int, vector<DeviceInfo>::iterator> websocket::deviceId2DeviceInfoIter;

mutex websocket::deviceIdMutex;
mutex websocket::estateMutex;
mutex websocket::deviceArrayMutex;
void websocket::modifyEstateIdOfDevice(int deviceId, int newEstateId)
{
    int oldEstateId = -1;
    vector<DeviceInfo>::iterator deviceInfoIt;
    {
        lock_guard<mutex> lg{deviceIdMutex};
        if (deviceId2DeviceInfoIter.count(deviceId) == 0)
        {
            return;
        }
        deviceInfoIt = deviceId2DeviceInfoIter[deviceId];
        oldEstateId = deviceInfoIt->estateId;
        deviceInfoIt->estateId = newEstateId;
    }
    lock_guard<mutex> lg{estateMutex};
    auto &arr = estateId2DeviceArray[oldEstateId];
    for (auto it = arr.begin(); it != arr.end(); it += 1)
    {
        if ((*it)->deviceId == deviceId)
        {
            arr.erase(it);
            break;
        }
    }
    estateId2DeviceArray[newEstateId].emplace_back(deviceInfoIt);
}
void websocket::addNewDevice(int deviceId, int estateId)
{
    vector<DeviceInfo>::iterator it;
    {
        lock_guard<mutex> lg{deviceArrayMutex};
        deviceInfoArray.emplace_back();
        deviceInfoArray.back().deviceId = deviceId;
        deviceInfoArray.back().estateId = estateId;
        it = prev(deviceInfoArray.end());
    }
    lock_guard<mutex> lg{estateMutex};
    estateId2DeviceArray[estateId].emplace_back(it);
}
json errorJsonMessage(std::string emsg)
{
    return {{"type", "error"}, {"errorMsg", emsg}};
}
json websocket::getDeviceInfoByEstateId(int estateId)
{
    json ret;
    vector<vector<DeviceInfo>::iterator>::iterator it, end;
    {
        lock_guard<mutex> lg{estateMutex};
        it = estateId2DeviceArray[estateId].begin();
        end = estateId2DeviceArray[estateId].end();
    }
    while (it != end)
    {
        ret.push_back({{"deviceId", (**it).deviceId}, {"state", (**it).state}});
        it += 1;
    }

    return ret;
}
pair<weak_ptr<ReceiverInfo>, RequestDescription> websocket::getRequestInfoById(int requestId)
{
    lock_guard<mutex> lg{requestMutex};
    pair<weak_ptr<ReceiverInfo>, RequestDescription> elem;
    for (auto it = requestQueue.begin(); it != requestQueue.end();)
    {
        auto _rec = it->first.lock();
        if (_rec and _rec->requestId == it->second.requestId)
        {
            if (it->second.requestId == requestId)
            {
                auto ret = *it;
                requestQueue.erase(it);
                return ret;
            }
            it += 1;
        }
        else // request invalidation
        {
            it = requestQueue.erase(it);
            continue;
        }
    }
    return {shared_ptr<ReceiverInfo>(), RequestDescription()}; // return nullptr;
}
json websocket::getRequestList()
{
    lock_guard<mutex> lg{requestMutex};
    auto it = requestQueue.begin();
    json jvalue = {};
    while (it != requestQueue.end())
    {
        shared_ptr<ReceiverInfo> _receiverInfoPtr;
        if ((_receiverInfoPtr = it->first.lock()) == nullptr)
        {
            printf("\t--nullptr\n");
            it = requestQueue.erase(it);
            continue;
        }
        jvalue.push_back(
            {{Parameter::REQUEST_ID, _receiverInfoPtr->requestId},
             {Parameter::ESTATE_ID, _receiverInfoPtr->estateIdSelected}});
        it += 1;
    }
    printf("[Current Request List]:%s\n", jvalue.dump().c_str());
    return jvalue;
}
int websocket::requestEmplaceBack(shared_ptr<ReceiverInfo> _receiverInfoPtr)
{
    {
        lock_guard<mutex> lg{_receiverInfoPtr->itemMutex};
        _receiverInfoPtr->requestId = requestId.fetch_add(1);
        {
            lock_guard<mutex> lg{requestMutex};
            requestQueue.push_back({_receiverInfoPtr, RequestDescription()});
            requestQueue.back().second.requestId = _receiverInfoPtr->requestId;
        }
    }
    return _receiverInfoPtr->requestId;
}
bool websocket::deleteRequestById(int requestId)
{
    lock_guard<mutex> lg{requestMutex};
    for (auto it = requestQueue.begin(); it != requestQueue.end(); it += 1)
    {
        if (it->second.requestId == requestId)
        {
            printf("[custom service request]: delete-id:%d\n", requestId);
            requestQueue.erase(it);
            return true;
        }
    }
    return false;
}
void websocket::handleNewConnection(const HttpRequestPtr &req, const WebSocketConnectionPtr &wsConnPtr)
{
    std::string requestPath = req->getPath();
    if (requestPath == Path::WebSocketPath::SENDER_CONNECT_PATH)
    {
        std::string rawDeviceId = req->getParameter(Parameter::DEVICE_ID);
        int deviceId = atoi(rawDeviceId.c_str());
        if (deviceId == 0) // invalid;
        {
            return;
        }
        {
            if constexpr (debug)
            {
                printf("[Device-Connection]device id: %d\n", deviceId);
            }

            decltype(deviceId2DeviceInfoIter)::iterator it;
            {
                lock_guard<mutex> lg{deviceIdMutex};
                if ((it = deviceId2DeviceInfoIter.find(deviceId)) == deviceId2DeviceInfoIter.end())
                {
                    return;
                }
            }
            {
                lock_guard<mutex> lg{senderMutex};
                senderPtr2DeviceInfoIter[wsConnPtr] = it->second;
            }
            {
                lock_guard<mutex> lg{it->second->itemMutex};
                it->second->senderPtr = wsConnPtr;
                it->second->state = DeviceVideoConnectionStates::FREE;
            }
        }
    }
    else if (requestPath == Path::WebSocketPath::RECEIVER_CONNECT_PATH)
    {
        if (req->session()->find("isLogin") == false or req->session()->get<bool>("isLogin") == false)
        {
            wsConnPtr->send(errorJsonMessage("userNotLogin").dump());
            return;
        }
        std::string rawEstateId = req->getParameter(Parameter::ESTATE_ID);
        int estateId = atoi(rawEstateId.c_str());
        {
            lock_guard<mutex> lg{estateMutex};
            if (estateId2DeviceArray.count(estateId) == 0)
            {
                return;
            }
        }
        {
            if constexpr (debug)
            {
                printf("[Receiver]:receiver-connect-estate-id:%d\n", estateId);
            }
            shared_ptr<ReceiverInfo> newReceiverPtr = std::make_shared<ReceiverInfo>();
            newReceiverPtr->receiverPtr = wsConnPtr;
            newReceiverPtr->estateIdSelected = estateId;
            newReceiverPtr->customServiceRequestState = CustomServiceStates::USER_NO_DEVICE_CONNECTION;
            lock_guard<mutex> lg{receiverMutex};
            receiverPtr2receiverInfo[wsConnPtr] = newReceiverPtr;
        }
        wsConnPtr->send((json){{"type", "deviceInfo"}, {"deviceInfo", getDeviceInfoByEstateId(estateId)}}
                            .dump());
    }
    else if (requestPath == Path::WebSocketPath::CUSTOM_SERVICE_CONNECT_PATH)
    {
        if (req->session()->find(SessionCookie::IS_CUSTOM_SERVICE) == false)
        {
            return;
        }
        {
            if constexpr (debug)
            {
                printf("[Service-Stuff-Connect]:%d\n", req->session()->get<int>("managerId"));
            }
            lock_guard<mutex> lg{customServiceMutex};
            customServicePtr2ServiceInfo[wsConnPtr];
        }
    }
}
void websocket::handleNewMessageFromSender(json &jmsg, std::string &&message, WebSocketConnectionPtr wsConnPtr)
{
    if (jmsg.contains("to") == false)
    {
        if (jmsg["type"] == MessageType::Device::STOP_VIDEO)
        {
            int _c_id = -1;
            int _r_id = -1;
            int _d_id = -1;
            shared_ptr<ReceiverInfo> receiverInfoPtr = nullptr;
            {
                lock_guard<mutex> lg{senderMutex};
                if (senderPtr2DeviceInfoIter.count(wsConnPtr) == 0)
                {
                    return;
                }
                receiverInfoPtr = senderPtr2DeviceInfoIter[wsConnPtr]->receiverInfoPtr.lock();
                senderPtr2DeviceInfoIter[wsConnPtr]->state = DeviceVideoConnectionStates::FREE;
                senderPtr2DeviceInfoIter[wsConnPtr]->receiverInfoPtr.reset();
            }
            WebSocketConnectionPtr customServicePtr = nullptr;
            WebSocketConnectionPtr receiverPtr = nullptr;
            if (receiverInfoPtr)
            {
                lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
                if constexpr (debug)
                {
                    _c_id = receiverInfoPtr->connectionId;
                    _r_id = receiverInfoPtr->requestId;
                    _d_id = receiverInfoPtr->deviceId;
                }
                receiverInfoPtr->requestId = -1;
                receiverInfoPtr->deviceId = -1;
                receiverInfoPtr->connectionId = -1;
                receiverPtr = receiverInfoPtr->receiverPtr.lock();
                receiverInfoPtr->senderPtr.reset();
                customServicePtr = receiverInfoPtr->customServicePtr.lock();
                receiverInfoPtr->customServicePtr.reset();
                receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_NO_REQUEST;
            }
            receiverPtr->send((json){
                {"type", MessageType::Device::CLOSE_CONNECTION},
                {"connectionId", _c_id}}
                                  .dump());
            if (customServicePtr)
            {
                {
                    lock_guard<mutex> lg{customServiceMutex};
                    if (customServicePtr2ServiceInfo.count(customServicePtr) == 0)
                    {
                        return;
                    }
                    customServicePtr2ServiceInfo[customServicePtr].requestId = -1;
                    customServicePtr2ServiceInfo[customServicePtr].receiverInfoPtr.reset();
                    customServicePtr2ServiceInfo[customServicePtr].customServiceState =
                        CustomServiceStates::CUSTOM_SERVICE_FREE;
                }
                customServicePtr->send((json){
                    {"type", MessageType::Device::CLOSE_CONNECTION},
                    {"connectionId", _c_id}}
                                           .dump());
            }
            if constexpr (debug)
            {
                printf("[sender stop video] d: %d; c: %d; r: %d\n", _d_id, _c_id, _r_id);
            }
        }
        else if (jmsg["type"] == MessageType::Device::LOCATION)
        {
            shared_ptr<ReceiverInfo> receiverInfoPtr = nullptr;
            {
                lock_guard<mutex> lg{senderMutex};
                if (senderPtr2DeviceInfoIter.count(wsConnPtr) == 0)
                {
                    return;
                }
                receiverInfoPtr = senderPtr2DeviceInfoIter[wsConnPtr]->receiverInfoPtr.lock();
            }
            if (receiverInfoPtr)
            {
                auto receiverWebSocketConnectionPtr = receiverInfoPtr->receiverPtr.lock();
                if (receiverWebSocketConnectionPtr)
                    receiverWebSocketConnectionPtr->send(message);
            }
        }
        else if (jmsg["type"] == MessageType::Device::DEVICE_STATE)
        {
            if (jmsg["state"] == "free")
            {
                vector<DeviceInfo>::iterator it;
                {
                    lock_guard<mutex> lg{senderMutex};
                    if (senderPtr2DeviceInfoIter.count(wsConnPtr) == false)
                    {
                        return;
                    }
                    it = senderPtr2DeviceInfoIter[wsConnPtr];
                }
                {
                    lock_guard<mutex> lg(it->itemMutex);
                    it->state = DeviceVideoConnectionStates::FREE;
                }
            }
        }
    }
    else if (jmsg["to"] == TO_RECEIVER)
    {
        int _d_id = -1, _c_id = -1, _r_id = -1;
        shared_ptr<ReceiverInfo> receiverInfoPtr = nullptr;
        {
            lock_guard<mutex> lg{senderMutex};
            if (senderPtr2DeviceInfoIter.count(wsConnPtr) == 0)
            {
                return;
            }
            receiverInfoPtr = senderPtr2DeviceInfoIter[wsConnPtr]->receiverInfoPtr.lock();
        }
        if (receiverInfoPtr)
        {
            WebSocketConnectionPtr receiverWebSocketConnectionPtr = receiverInfoPtr->receiverPtr.lock();
            if (receiverWebSocketConnectionPtr)
            {
                receiverWebSocketConnectionPtr->send(message);
            }
            _d_id = receiverInfoPtr->deviceId;
            _r_id = receiverInfoPtr->requestId;
            _c_id = receiverInfoPtr->connectionId;
        }
        if constexpr (debug)
        {
            printf("[sender to receiver message] d: %d; c: %d; r: %d, type: %s\n", _d_id, _c_id, _r_id, jmsg["type"].get<std::string>().c_str());
        }
    }
    else if (jmsg["to"] == TO_CUSTOM_SERVICE)
    {
        int _d_id = -1, _c_id = -1, _r_id = -1;
        shared_ptr<ReceiverInfo> receiverInfoPtr = nullptr;
        {
            lock_guard<mutex> lg{senderMutex};
            if (senderPtr2DeviceInfoIter.count(wsConnPtr) == 0)
            {
                return;
            }
            receiverInfoPtr = senderPtr2DeviceInfoIter[wsConnPtr]->receiverInfoPtr.lock();
        }
        WebSocketConnectionPtr customServicePtr;
        if (receiverInfoPtr)
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            {
                _c_id = receiverInfoPtr->connectionId;
                _r_id = receiverInfoPtr->requestId;
                _d_id = receiverInfoPtr->deviceId;
            }
            customServicePtr = receiverInfoPtr->customServicePtr.lock();
        }
        if (customServicePtr)
        {
            customServicePtr->send(message);
        }
        if constexpr (debug)
        {
            printf("[sender to service message] d: %d; c: %d; r: %d\n,type-%s", _d_id, _c_id, _r_id, jmsg["type"].get<std::string>().c_str());
        }
    }
}
void websocket::handleNewMessageFromReceiver(json &jmsg, std::string &&message, WebSocketConnectionPtr wsConnPtr)
{
    shared_ptr<ReceiverInfo> receiverInfoPtr;
    {
        if (receiverPtr2receiverInfo.count(wsConnPtr) == 0)
        {
            printf("[Error] Unregistered WebsocketPtr\n");
            return;
        }
        else
        {
            receiverInfoPtr = receiverPtr2receiverInfo[wsConnPtr];
        }
    }
    if (jmsg.contains("to") == true)
    {
        int _c_id = -1;
        WebSocketConnectionPtr _sender = 0, _service = 0;
        if (jmsg["to"] == TO_CUSTOM_SERVICE)
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            _service = receiverInfoPtr->customServicePtr.lock();
            _c_id = receiverInfoPtr->connectionId;
        }
        else if (jmsg["to"] == TO_SENDER)
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            _sender = receiverInfoPtr->senderPtr.lock();
            _c_id = receiverInfoPtr->connectionId;
        }
        if (_sender)
        {
            _sender->send(message);
            if constexpr (debug)
                printf("[receiver to sender message] c:%d; type: %s\n", _c_id, jmsg["type"].get<std::string>().c_str());
        }
        else if (_service)
        {
            _service->send(message);
            if constexpr (debug)
                printf("[receiver to custom service message] c:%d; type:%s\n", _c_id, jmsg["type"].get<std::string>().c_str());
        }
    }
    else if (jmsg["type"] == MessageType::User::CLOSE_VIDEO_CONNECTION)
    {
        int userSentConnectionId = -1;
        int connectionDeviceId = -1;
        if (jmsg.count("connectionId") == 0)
        {
            return;
        }
        userSentConnectionId = jmsg["connectionId"].get<int>();
        WebSocketConnectionPtr _sender = 0, _service = 0;
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            if (userSentConnectionId != receiverInfoPtr->connectionId)
            {
                return;
            }
            _service = receiverInfoPtr->customServicePtr.lock();
            _sender = receiverInfoPtr->senderPtr.lock();
            connectionDeviceId = receiverInfoPtr->deviceId;
            receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_NO_DEVICE_CONNECTION;
            receiverInfoPtr->deviceId = -1;
            receiverInfoPtr->connectionId = -1;
            receiverInfoPtr->requestId = -1;
            receiverInfoPtr->senderPtr.reset();
            receiverInfoPtr->customServicePtr.reset();
        }
        if constexpr (debug)
        {
            printf("[receiver-close-video], connection-id=%d\n", userSentConnectionId);
        }
        vector<DeviceInfo>::iterator deviceInfo;
        {
            lock_guard<mutex> lg{deviceIdMutex};
            deviceInfo = deviceId2DeviceInfoIter[connectionDeviceId];
        }
        {
            if (_sender)
            {
                deviceInfo->receiverInfoPtr.reset();
                deviceInfo->state = DeviceVideoConnectionStates::FREE;
                _sender->send(message);
            }
        }
        if (_service)
        {
            _service->send(message);
        }
    }
    else if (jmsg["type"] == MessageType::User::CUSTOM_SERVICE_REQUEST)
    {
        {
            WebSocketConnectionPtr _receiver;
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            if (receiverInfoPtr->customServiceRequestState != CustomServiceStates::USER_NO_REQUEST)
            {
                _receiver = receiverInfoPtr->receiverPtr.lock();
                if (_receiver)
                {
                    _receiver->send(errorJsonMessage(MessageType::User::REPEATED_CUSTOM_SERVICE_REQUEST).dump());
                }
                return;
            }
        }
        int _requestIdRet = requestEmplaceBack(receiverInfoPtr);
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            WebSocketConnectionPtr _receiverWebSocketConnection;
            receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_REQUESTING;
            _receiverWebSocketConnection = receiverInfoPtr->receiverPtr.lock();
            if (_receiverWebSocketConnection)
            {
                _receiverWebSocketConnection->send(
                    (json){
                        {"type", MessageType::User::REQUEST_CUSTOM_SERVICE_REQUEST_SUCCEED},
                        {Parameter::REQUEST_ID, _requestIdRet}}
                        .dump());
            }
        }
    }
    else if (jmsg["type"] == MessageType::User::STOP_CUSTOM_SERVICE)
    {
        WebSocketConnectionPtr _sender = nullptr, _service = nullptr;
        int _requestId = -1;
        int _connectionId = -1;
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            _requestId = receiverInfoPtr->requestId;
            _connectionId = receiverInfoPtr->connectionId;
            _sender = receiverInfoPtr->senderPtr.lock();
            _service = receiverInfoPtr->customServicePtr.lock();
            receiverInfoPtr->customServicePtr.reset();
            receiverInfoPtr->requestId = -1;
            receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_NO_REQUEST;
        }
        bool ret = deleteRequestById(requestId);
        printf("[delete-request] request_id%d,success:%d\n", _requestId, (int)ret);
        // if request arent answered by service stuff, remove from list;
        if (_sender)
        {
            _sender->send(
                (json){
                    {"type", MessageType::User::STOP_CUSTOM_SERVICE},
                    {Parameter::CONNECTION_ID, _connectionId},
                    {Parameter::REQUEST_ID, _requestId}}
                    .dump());
        }
        if (_service)
        {
            _service->send(
                (json){
                    {"type", MessageType::User::STOP_CUSTOM_SERVICE},
                    {Parameter::CONNECTION_ID, _connectionId},
                    {Parameter::REQUEST_ID, _requestId}}
                    .dump());
        }
    }
    else if (jmsg["type"] == MessageType::User::ESTABLISH_CONNECTION)
    {
        if (jmsg.count(Parameter::DEVICE_ID) == 0)
        {
            return;
        }
        int deviceId = jmsg[Parameter::DEVICE_ID].get<int>();
        int invalidRequest = 0;
        int _connectionId = -1;
        WebSocketConnectionPtr _sender = nullptr;
        vector<DeviceInfo>::iterator it;
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            if (receiverInfoPtr->customServiceRequestState != CustomServiceStates::USER_NO_DEVICE_CONNECTION)
            {
                wsConnPtr->send(errorJsonMessage("repeatedConnection").dump());
                return; // repeatedConnection
            }
            {
                {
                    lock_guard<mutex> lg1{deviceIdMutex};
                    if (deviceId2DeviceInfoIter.count(deviceId) == 0)
                    {
                        // wsConnPtr->send(errorJsonMessage("wrongDeviceId").dump());
                        return;
                    }
                    else
                    {
                        it = deviceId2DeviceInfoIter[deviceId];
                    }
                }
                {
                    lock_guard<mutex> lg1{it->itemMutex};
                    if (it->state != DeviceVideoConnectionStates::FREE)
                    {
                        wsConnPtr->send(errorJsonMessage("deviceOccupied").dump());
                        return;
                    }
                    _sender = it->senderPtr.lock();
                    it->state = DeviceVideoConnectionStates::OCCUPIED;
                    it->receiverInfoPtr = receiverInfoPtr;
                }
                receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_NO_REQUEST;
                receiverInfoPtr->connectionId = connectionId.fetch_add(1);
                _connectionId = receiverInfoPtr->connectionId;
                receiverInfoPtr->deviceId = deviceId;
                receiverInfoPtr->senderPtr = _sender;
            }
        }
        wsConnPtr->send(
            (json){
                {"type", "establishConnectionSucceed"},
                {"connectionId", _connectionId}}
                .dump());
        if (_sender)
        {
            _sender->send(
                (
                    (json){
                        {"type", MessageType::User::ESTABLISH_CONNECTION},
                        {"connectionId", _connectionId}})
                    .dump());
        }
    }
    else if (jmsg["type"] == MessageType::User::GET_DEVICE_INFO)
    {
        int estateId = -1;
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            estateId = receiverInfoPtr->estateIdSelected;
        }
        json deviceInfoJsonArray = getDeviceInfoByEstateId(estateId);
        wsConnPtr->send((json){
            {"type", "deviceInfo"},
            {"deviceInfo", deviceInfoJsonArray}}
                            .dump());
    }
}
void websocket::handleNewMessageFromCustomService(json &jmsg, std::string &&message, WebSocketConnectionPtr wsConnPtr)
{
    {
        lock_guard<mutex> lg{customServiceMutex};
        if (auto it = customServicePtr2ServiceInfo.find(wsConnPtr); it == customServicePtr2ServiceInfo.end())
        {
            return;
        }
    }
    if (jmsg.contains("to") == 0)
    {
        shared_ptr<ReceiverInfo> _receiverInfoPtr = nullptr;
        if (jmsg["type"] == MessageType::CustomService::ACCEPT_REQUEST)
        {
            if (jmsg.contains(Parameter::REQUEST_ID) == 0 /* or jmsg.contains(Parameter::CONNECTION_ID) == 0 */)
            {
                return;
            }
            int requestId = jmsg[Parameter::REQUEST_ID].get<int>();
            // int connectionId = jmsg[Parameter::CONNECTION_ID].get<int>();
            auto _req = getRequestInfoById(requestId);
            _receiverInfoPtr = _req.first.lock();
            WebSocketConnectionPtr _sender_ptr = nullptr;
            if (_receiverInfoPtr)
            {
                lock_guard<mutex> lg1{_receiverInfoPtr->itemMutex};
                if (_receiverInfoPtr->customServiceRequestState != CustomServiceStates::USER_REQUESTING)
                {
                    wsConnPtr->send(errorJsonMessage(MessageType::CustomService::REQUEST_INVALIDATION).dump());
                    return;
                }
                {
                    lock_guard<mutex> lg2{customServiceMutex};
                    _receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_BEING_SERVICED;
                    _receiverInfoPtr->customServicePtr = wsConnPtr;
                    customServicePtr2ServiceInfo[wsConnPtr].requestId = requestId;
                    customServicePtr2ServiceInfo[wsConnPtr].receiverInfoPtr = _receiverInfoPtr;
                    customServicePtr2ServiceInfo[wsConnPtr].customServiceState = CustomServiceStates::CUSTOM_SERVICE_ANSWERING;
                    _sender_ptr = _receiverInfoPtr->senderPtr.lock();
                }
            }
            else
            {
                wsConnPtr->send(errorJsonMessage(MessageType::CustomService::REQUEST_INVALIDATION).dump());
                return;
            }
            if (_sender_ptr)
            {
                _sender_ptr->send(
                    (json){
                        {"type", MessageType::CustomService::ACCEPT_REQUEST},
                        {"requestId", requestId}}
                        .dump());
                auto rcvrWSConnPtr = _receiverInfoPtr->receiverPtr.lock();
                if (rcvrWSConnPtr)
                {
                    rcvrWSConnPtr->send(
                        (json){
                            {"type", MessageType::CustomService::ACCEPT_REQUEST},
                            {"requestId", requestId}}
                            .dump());
                }
            }
            wsConnPtr->send(
                (json){
                    {"type", MessageType::CustomService::ACCEPT_REQUEST_SUCCEED},
                    {"requestId", requestId}}
                    .dump());
            deleteRequestById(requestId);
        }
        else if (jmsg["type"] == MessageType::CustomService::STOP_SERVICE)
        {
            if (jmsg.count(Parameter::REQUEST_ID) == 0 or jmsg.count(Parameter::CONNECTION_ID) == 0)
            {
                return;
            }
            int requestId = jmsg[Parameter::REQUEST_ID].get<int>();
            int connectionId = jmsg[Parameter::CONNECTION_ID].get<int>();
            shared_ptr<ReceiverInfo> receiverInfoPtr = nullptr;
            WebSocketConnectionPtr _sender = nullptr, _receiver = nullptr;
            {
                lock_guard<mutex> lg{customServiceMutex};
                if (auto it = customServicePtr2ServiceInfo.find(wsConnPtr); it == customServicePtr2ServiceInfo.end())
                {
                    return;
                }
                else
                {
                    receiverInfoPtr = it->second.receiverInfoPtr.lock();
                    it->second.requestId = -1;
                    it->second.receiverInfoPtr.reset();
                    it->second.customServiceState = CustomServiceStates::CUSTOM_SERVICE_FREE;
                }
            }
            if (receiverInfoPtr)
            {
                lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
                if (connectionId != receiverInfoPtr->connectionId or requestId != receiverInfoPtr->requestId)
                {
                    return;
                }
                _sender = receiverInfoPtr->senderPtr.lock();
                _receiver = receiverInfoPtr->receiverPtr.lock();
                receiverInfoPtr->requestId = -1;
                receiverInfoPtr->customServicePtr.reset();
                receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_NO_REQUEST;
            }
            if (_sender)
            {
                _sender->send(message);
            }
            if (_receiver)
            {
                _receiver->send(message);
            }
        }
        else if (jmsg["type"] == MessageType::CustomService::GET_REQUESTLIST)
        {
            printf("[Service]: get request list\n");
            json requestList = getRequestList();
            wsConnPtr->send(
                (json){
                    {"type", MessageType::CustomService::REQUEST_LIST},
                    {"data", requestList}}
                    .dump());
        }
    }
    else
    {
        std::string toWho = jmsg["to"];
        WebSocketConnectionPtr _sender = nullptr, _receiver = nullptr;
        {
            lock_guard<mutex> lg{customServiceMutex};
            if (auto it = customServicePtr2ServiceInfo.find(wsConnPtr); it != customServicePtr2ServiceInfo.end())
            {
                auto _recv_info = it->second.receiverInfoPtr.lock();
                if (!_recv_info)
                {
                    return;
                }
                lock_guard<mutex> lg{_recv_info->itemMutex};
                if (toWho == TO_SENDER)
                    _sender = _recv_info->senderPtr.lock();
                if (toWho == TO_RECEIVER)
                    _receiver = _recv_info->receiverPtr.lock();
            }
        }
        if (_sender)
        {
            _sender->send(message);
        }
        if (_receiver)
        {
            _receiver->send(message);
        }
    }
}
void websocket::handleNewMessage(const WebSocketConnectionPtr &wsConnPtr, std::string &&message, const WebSocketMessageType &type)
{

    if (type == WebSocketMessageType::Ping)
    {
        return;
    }
    if (message.size() <= 4)
    {
        return;
    }
    std::cout << message << std::endl;
    json jmsg = json::parse(message);
    if (jmsg.count("from") == 0)
    {
        return;
    }
    std::string from = jmsg["from"];
    if (from == "sender")
    {
        handleNewMessageFromSender(jmsg, std::forward<std::string &&>(message), wsConnPtr);
    }
    else if (from == "receiver")
    {
        handleNewMessageFromReceiver(jmsg, std::forward<std::string &&>(message), wsConnPtr);
    }
    else if (from == "customService")
    {
        handleNewMessageFromCustomService(jmsg, std::forward<std::string &&>(message), wsConnPtr);
    }
}

void websocket::handleConnectionClosed(const WebSocketConnectionPtr &wsConnPtr)
{
    int identity = 0; // sender ;
    shared_ptr<ReceiverInfo> receiverInfoPtr = nullptr;
    vector<DeviceInfo>::iterator deviceInfoIterator;
    WebSocketConnectionPtr _service = nullptr;
    WebSocketConnectionPtr _recv = nullptr;
    WebSocketConnectionPtr _sender = nullptr;
    {
        lock_guard<mutex> lg{senderMutex};
        int deviceId;
        if (senderPtr2DeviceInfoIter.count(wsConnPtr) == 0)
        {
            identity = 1;
        }
        else
        {
            receiverInfoPtr = senderPtr2DeviceInfoIter[wsConnPtr]->receiverInfoPtr.lock();
            deviceInfoIterator = senderPtr2DeviceInfoIter[wsConnPtr];
            senderPtr2DeviceInfoIter.erase(wsConnPtr);
        }
    }
    if (identity == 0) // sender;
    {
        if constexpr (debug)
        {
            printf("[close connection]:sender\n");
        }
        int curConnectionId = -1;
        int curRequestId = -1;
        {
            lock_guard<mutex> lg{deviceInfoIterator->itemMutex};
            curConnectionId = deviceInfoIterator->connectionId;
            deviceInfoIterator->connectionId = -1;
            deviceInfoIterator->senderPtr.reset();
            deviceInfoIterator->state = DeviceVideoConnectionStates::DISCONNECTED;
        }
        if (receiverInfoPtr)
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            if (receiverInfoPtr->connectionId == curConnectionId)
            {
                curRequestId = receiverInfoPtr->requestId;
                _service = receiverInfoPtr->customServicePtr.lock();
                _recv = receiverInfoPtr->receiverPtr.lock();
                receiverInfoPtr->connectionId = -1;
                receiverInfoPtr->requestId = -1;
                receiverInfoPtr->deviceId = -1;
                receiverInfoPtr->customServicePtr.reset();
                receiverInfoPtr->senderPtr.reset();
                receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_NO_DEVICE_CONNECTION;
            }
        }
        if (_service)
        {
            _service->send(
                (json){
                    {"type", MessageType::Device::CLOSE_CONNECTION},
                    {"connectionId", curConnectionId}}
                    .dump());
        }
        if (_recv)
        {
            _recv->send(
                (json){
                    {"type", MessageType::Device::CLOSE_CONNECTION},
                    {"requestId", curRequestId}}
                    .dump());
        }
    }
    else if (identity >= 1)
    {
        {
            lock_guard<mutex> lg{receiverMutex};
            auto _recv_it =
                receiverPtr2receiverInfo.find(wsConnPtr);
            if (_recv_it == receiverPtr2receiverInfo.end())
            {
                identity = 2;
            }
            else
            {
                receiverInfoPtr = _recv_it->second;
                receiverPtr2receiverInfo.erase(_recv_it);
            }
        }
        if (identity == 1) // receiver
        {
            lock_guard<mutex> lg{receiverInfoPtr->itemMutex};
            _sender = receiverInfoPtr->senderPtr.lock();
            _service = receiverInfoPtr->customServicePtr.lock();
            int curConnectionId = receiverInfoPtr->connectionId;
            int curRequestId = receiverInfoPtr->requestId;
            receiverInfoPtr.reset();
            if constexpr (debug)
                printf("[receiver-close-connection] shared_ptr<ReceiverInfo>::use_count:%ld, connectionId=%d;\n", receiverInfoPtr.use_count(), curConnectionId);
            lg.~lock_guard();
            if (_sender)
            {
                _sender->send(
                    (json){
                        {"type", MessageType::User::CLOSE_VIDEO_CONNECTION},
                        {"connectionId", curConnectionId}}
                        .dump());
            }
            if (_service)
            {
                _service->send(
                    (json){
                        {"type", MessageType::User::CLOSE_VIDEO_CONNECTION},
                        {"requestId", curRequestId}}
                        .dump());
            }
        }
    }
    if (identity >= 2)
    {
        int curRequestId = -1;
        int curConnectionId = -1;
        {
            lock_guard<mutex> lg{customServiceMutex};
            auto _it = customServicePtr2ServiceInfo.find(wsConnPtr);
            if (_it == customServicePtr2ServiceInfo.end())
            {
                identity = 3;
                return;
            }
            else
            {
                receiverInfoPtr = customServicePtr2ServiceInfo[wsConnPtr].receiverInfoPtr.lock();
                customServicePtr2ServiceInfo.erase(_it);
            }
        }
        if (identity == 2) // custom service
        {
            if (receiverInfoPtr)
            {
                lock_guard lg{receiverInfoPtr->itemMutex};
                _recv = receiverInfoPtr->receiverPtr.lock();
                _sender = receiverInfoPtr->senderPtr.lock();
                curRequestId = receiverInfoPtr->requestId;
                curConnectionId = receiverInfoPtr->connectionId;
                receiverInfoPtr->customServiceRequestState = CustomServiceStates::USER_NO_REQUEST;
                receiverInfoPtr->customServicePtr.reset();
                receiverInfoPtr->requestId = -1;
            }
            if (_recv)
            {
                _recv->send(
                    (json){
                        {"type", MessageType::CustomService::STOP_SERVICE},
                        {"connectionId", curConnectionId},
                        {"requestId", curRequestId}}
                        .dump());
            }
            if (_sender)
            {
                _sender->send(
                    (json){
                        {"type", MessageType::CustomService::STOP_SERVICE},
                        {"connectionId", curConnectionId},
                        {"requestId", curRequestId}}
                        .dump());
            }
        }
    }
}