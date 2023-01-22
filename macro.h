#pragma once
#include <string>
#include <iostream>
const bool debug = true;

#define TO_SENDER "sender"
#define TO_RECEIVER "receiver"
#define TO_CUSTOM_SERVICE "customService"
#define DEVICE_INFO_LIST_NUMBER_ITEM 10
namespace SessionCookie
{
    const std::string IS_CUSTOM_SERVICE{"isCustomService"};
    const std::string IS_MAINTENANCE{"isMaintenance"};
    const std::string MODIFY_MANAGER_PRIVILEGE_SUCCESSFULLY{"modifySuccessfully"};
    const std::string MODIFY_ESTATE_SUCCESSFULLY{"modifyEstateInfoSuccessfully"};
    const std::string CREATE_NEW_ESTATE_SUCCESSFULLY{"createNewEstateSuccessfully"};
    const std::string MODIFY_DEVICE_INFO_SUCCESSFULLY{"modifyDeviceInfoSuccessfully"};
    const std::string ADD_NEW_DEVICE_SUCCESSFULLY{"addNewDeviceSuccessfully"};
    const std::string NEW_DEVICE_ID{"newDeviceId"};
    const std::string MANAGER_ID{"managerId"};
};
namespace MessageType
{
    namespace Device
    {
        const std::string STOP_VIDEO{"stopVideo"};
        const std::string DEVICE_STATE{"deviceState"};
        const std::string CLOSE_CONNECTION{"closeConnection"};
        const std::string LOCATION{"location"};
    };

    namespace User
    {
        const std::string CLOSE_VIDEO_CONNECTION{"userCloseVideoConnection"};
        const std::string STOP_CUSTOM_SERVICE{"userStopCustomServiceRequest"};
        const std::string CUSTOM_SERVICE_REQUEST{"customServiceRequest"};
        const std::string ESTABLISH_CONNECTION{"establishConnection"};
        const std::string REQUEST_CUSTOM_SERVICE_REQUEST_SUCCEED{"userRequestCustomServiceSucceed"};
        const std::string REPEATED_CUSTOM_SERVICE_REQUEST{"repeatedCustomSerivceRequest"};
        const std::string GET_DEVICE_INFO{"getDeviceInfo"};
    };
    namespace CustomService
    {
        const std::string STOP_SERVICE{"customServiceStopCustomService"};
        const std::string ACCEPT_REQUEST{"acceptCustomServiceRequest"};
        const std::string GET_REQUESTLIST{"getRequestList"};
        const std::string REQUEST_INVALIDATION{"customServiceRequestInvalidation"};
        const std::string ACCEPT_REQUEST_SUCCEED{"customServiceAcceptRequestSucceed"};
        const std::string REQUEST_LIST{"customServiceRequestList"};
    };

};
namespace Parameter
{
    const std::string CONNECTION_ID{"connectionId"};
    const std::string DEVICE_ID{"deviceId"};
    const std::string ESTATE_ID{"estateId"};
    const std::string REQUEST_ID{"requestId"};
};
namespace DeviceStates
{
    const std::string NORMAL{"normal"};
    const std::string FORBIDDEN{"forbidden"};
    const std::string BROKEN_DOWN{"broken_down"};
};
namespace DeviceVideoConnectionStates
{
    const int FREE = 0;
    const int DISCONNECTED = 1;
    const int OCCUPIED = 2;
};
namespace CustomServiceStates
{
    const int USER_NO_DEVICE_CONNECTION = 0,
              USER_NO_REQUEST = 1,
              USER_REQUESTING = 2,
              USER_BEING_SERVICED = 3,
              CUSTOM_SERVICE_FREE = 4,
              CUSTOM_SERVICE_ANSWERING = 5;
};
namespace Path
{
    namespace Default{
        
    };
    namespace WebSocketPath
    {
        const std::string RECEIVER_CONNECT_PATH = "/receiver/connect";
        const std::string SENDER_CONNECT_PATH = "/sender/connect";
        const std::string CUSTOM_SERVICE_CONNECT_PATH = "/customService/connect";
    };
    namespace FilePath
    {
        namespace Html
        {
            const std::string DEFAULT_MAIN_PAGE{"../front/index/defaultMainPage.html"};
            const std::string CREATE_NEW_ESTATE_INFO_PAGE{"../front/estate/createNewEstateInfo.html"};
            const std::string ESTATE_LIST_PAGE{"../front/estate/estateListPage.html"};
            const std::string SERVICE_MAIN_PAGE{"../front/service/serviceMainPage.html"};
            const std::string MANAGER_LOGIN_PAGE{"../front/manager/managerLoginPage.html"};
            const std::string MANAGER_MAIN_PAGE{"../front/manager/managerMainPage.html"};
            const std::string MANAGER_MANAGE_USERS_PAGE{"../front/mananger/manageUsersPage.html"};
            const std::string MANAGER_MODIFY_MANAGER_PAGE{"../front/manager/modifyManagerPrivilegePage.html"};
            const std::string MANAGER_CREATE_NEW_USER_PAGE{"../front/manager/managerCreateNewUserPage.html"};
            const std::string MANAGER_CREATE_NEW_MANAGER_PAGE{"../front/manager/managerCreateNewManagerPage.html"};
            const std::string CUSTOM_SERVICE_MAIN_PAGE{"../front/customService/customServicePage.html"};
            const std::string USER_SELECT_ESTATE_PAGE{"../front/user/selectEstatePage.html"};
            const std::string USER_LOGIN_PAGE{"../front/user/userLoginPage.html"};
            const std::string USER_MAIN_PAGE{"../front/user/userMainPage.html"};
            const std::string USER_VIDEO_DISPLAY_PAGE{"../front/user/userVideoDisplayPage.html"};
            const std::string DEVICE_INFO_MAIN_PAGE{"../front/device/deviceInfoPage.html"};
            const std::string DEVICE_ADD_NEW_PAGE{"../front/device/addNewDevicePage.html"};
        };
        namespace Javascript
        {
            const std::string USER_PEER_CONNECTION{"../front/scripts/userPeerConnection.js"};
            const std::string CUSTOM_SERVICE_PEER_CONNECTION{"../front/scripts/customServicePeerConnection.js"};
        };
    };

};