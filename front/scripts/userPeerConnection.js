var turnServerURL = "stun:173.255.200.200:3478";
var turnUserName = "";
var turnCredential = "";
window.AudioContext = window.AudioContext || window.webkitAudioContext;
var RTCPeerConnection = window.RTCPeerConnection || window.mozRTCPeerConnection || window.webkitRTCPeerConnection;
var RTCSessionDescription = window.mozRTCSessionDescription || window.webkitRTCSessionDescription || window.RTCSessionDescription;
var websocketConnectionURL = "/receiver/connect?estateId=";
if (!String.prototype.format) {
    String.prototype.format = function () {
        var args = arguments;
        return this.replace(/{(\d+)}/g, function (match, number) {
            return typeof args[number] != 'undefined'
                ? args[number]
                : match
                ;
        });
    };
}

class ConnectionController {
    webSocketConnection = null;
    videoPeerConnection = null;
    customServicePeerConnection = null;
    deviceId = -1;
    estateId = -1;
    connectionId = -1;
    requestId = -1;
    remoteVideo = null;
    remoteAudio = null;
    deviceInfoList = null;
    stopStreamingBtn = null;
    customServiceBtn = null;
    stopCustomServiceBtn = null;
    remoteAudio = null;
    refreshDeviceInfoBtn = null;
    registerObjects(objArr) {
        objArr.forEach(
            (pair) => {
                this[pair[0]] = pair[1];
                switch (pair[0]) {
                    case "stopCustomServiceBtn":
                        pair[1].onclick = () => {
                            this.stopCustomService(true);
                        };
                        break;
                    case "customServiceBtn":
                        pair[1].onclick = () => {
                            this.sendCustomServiceRequest(true);
                        };
                        break;
                    case "refreshDeviceInfoBtn":
                        pair[1].onclick = () => {
                            this.sendJsonMessage({
                                from: "receiver",
                                type: "getDeviceInfo"
                            });
                        }
                        break;
                    case "stopStreamingBtn":
                        pair[1].onclick = () => {
                            this.stopStreaming(true);
                        };
                        break;
                };
            }
        );
    }
    sendJsonMessage(jsonValue) {
        jsonValue["from"] = "receiver";
        console.log("[send message] send to server: ", jsonValue);
        this.webSocketConnection.send(JSON.stringify(jsonValue));
    }
    sendEstablishVideoConnectionRequest(deviceIdSelected) {
        let jmsg = {
            "type": "establishConnection",
            "deviceId": deviceIdSelected
        };
        this.videoPeerConnection = new VideoDisplayPeerConnection();
        this.deviceId = deviceIdSelected;
        this.sendJsonMessage(jmsg);
        this.videoPeerConnection.establishPeerConnection();
    }
    deviceInfoHtmlCreator = (elem, _disabled) => {
        return "<li>device_id:{0}; state:{1}; \
            <button id='device-id-{0}' {2}>connect</button></li>".format(
            elem.deviceId, elem.state, _disabled
        );
    };
    generateDeviceInfoListFromJson(deviceInfoArray) {
        this.deviceInfoList.innerHTML = "";
        deviceInfoArray.forEach(
            (elem) => {
                let _disabled = "disabled";
                if (elem.state == 0) {
                    _disabled = "";
                }
                this.deviceInfoList.innerHTML +=
                    this.deviceInfoHtmlCreator(elem, _disabled);
            });
    };
    addConnectDeviceRequestEventHandler(deviceInfoArray) {
        deviceInfoArray.forEach(
            (item) => {
                let _btn = document.getElementById("device-id-{0}".format(item.deviceId));
                _btn.onclick = () => {
                    this.sendEstablishVideoConnectionRequest(item.deviceId);
                };
            }
        );
    }

    stopStreaming(isInitiative) {
        this.remoteVideo.style.visibility = 'hidden';
        this.stopStreamingBtn.disabled = true;
        this.customServiceBtn.disabled = true;
        if (isInitiative) {
            let jmsg = {
                "type": "userCloseVideoConnection",
                "connectionId": this.connectionId
            };
            this.sendJsonMessage(jmsg);
        }
        if (this.videoPeerConnection)
            this.videoPeerConnection.closePeerConnection();
        if (this.customServicePeerConnection)
            this.customServicePeerConnection.closePeerConnection();
        this.videoPeerConnection = null;
        this.customServicePeerConnection = null;
    }
    stopCustomService(isInitiative) {
        if (isInitiative) {
            let jmsg = {
                "type": "userStopCustomServiceRequest",
                "requestId": this.requestId
            };
            this.sendJsonMessage(jmsg);
        }
        if (this.customServicePeerConnection)
            this.customServicePeerConnection.closePeerConnection();
        this.customServicePeerConnection = null;
        this.customServiceBtn.disabled = false;
        this.stopCustomServiceBtn.disabled = true;
    }
    sendControlMessage(instruction_msg) {
        let jmsg = {
            "type": "control",
            "instruction": instruction_msg,
            "to": "sender",
            "connectionId": this.connectionId
        };
        this.sendJsonMessage(jmsg);
    }
    sendCustomServiceRequest() {
        let jmsg = {
            "type": "customServiceRequest"
        };
        this.sendJsonMessage(jmsg);
        this.customServiceBtn.disabled = true;
        this.stopCustomServiceBtn.disabled = false;
    }

    handleNewMessageFromSender(jmsg) {
        switch (jmsg.type) {
            case "offer":
                let _videoOnTrackCallback = (trackEvent) => {
                    this.remoteVideo.srcObject = trackEvent.streams[0];
                    this.remoteVideo.play();
                    this.remoteVideo.style.visibility = 'visible';
                    this.customServiceBtn.disabled = false;
                };
                let _sendVideoAnswerCallback = (answer) => {
                    this.sendJsonMessage({
                        type: "answer",
                        data: answer,
                        "to": "sender",
                        "connectionId": this.connectionId
                    });

                };
                this.videoPeerConnection = new VideoDisplayPeerConnection();
                console.log("[debug]: ", this.videoPeerConnection);
                this.videoPeerConnection.establishPeerConnection(_sendVideoAnswerCallback, _videoOnTrackCallback);
                this.videoPeerConnection.setRemoteSDPAndCreateAnswer(jmsg.sdp);
                break;
        }
    }
    handleNewMessageFromCustomService(jmsg) {
        switch (jmsg.type) {
            case "offer":
                let _audioOnTrackCallback = (trackEvent) => {
                    console.log("[info]--AUDIO-- on track");
                    this.remoteAudio.srcObject = trackEvent.streams[0];
                    this.remoteAudio.play();
                    this.customServiceBtn.disabled = false;
                };
                let _sendAudioAnswerCallback = (answer) => {
                    this.sendJsonMessage({
                        "type": "answer",
                        "sdp": answer,
                        "to": "customService",
                        "requestId": this.requestId,
                        "connectionId": this.connectionId
                    });
                };
                this.customServicePeerConnection = new CustomServicePeerConnection();
                this.customServicePeerConnection.establishPeerConnection(_sendAudioAnswerCallback, _audioOnTrackCallback);
                this.customServicePeerConnection.setRemoteSDPAndCreateAnswer(jmsg.sdp);
                break;
        }
    }
    constructor() {
        websocketConnectionURL= "ws://"+window.location.host+ websocketConnectionURL;

        let _webSocketOnMessageCallback =
            (evt) => {
                if (evt.data === '' || evt.data === '\x00\x00\x00\x00') {
                    return;
                }
                let jmsg = JSON.parse(evt.data);
                console.log("[message]", jmsg);
                if (("from" in jmsg) == true) {
                    if (jmsg.from === "sender") {
                        this.handleNewMessageFromSender(jmsg);
                    }
                    else if (jmsg.from === "customService") {
                        this.handleNewMessageFromCustomService(jmsg);
                    }
                }
                else {
                    switch (jmsg.type) {
                        case "deviceInfo":
                            this.generateDeviceInfoListFromJson(jmsg.deviceInfo);
                            this.addConnectDeviceRequestEventHandler(jmsg.deviceInfo);
                            break;
                        case "error":
                            window.alert(jmsg.errorMsg);
                            break;
                        case "establishConnectionSucceed":
                            this.stopStreamingBtn.disabled = false;
                            this.connectionId = jmsg.connectionId;
                            break;
                        case "userRequestCustomServiceSucceed":
                            this.requestId = jmsg.requestId;
                            break;
                        case
                            "customServiceStopCustomService":
                            this.stopCustomService(false);
                    }
                }
            };
        this.webSocketConnection = new WebSocket(websocketConnectionURL + estateId);
        this.webSocketConnection.onopen = () => {
            console.log("[info] WebSocket Opened");
        };
        this.webSocketConnection.onmessage = _webSocketOnMessageCallback;
    }
};
class VideoDisplayPeerConnection {
    mediaConfiguration = {
        "OfferToReceiveAudio": false,
        "OfferToReceiveVideo": true
    };
    iceConfiguration = {
        "iceServers": [{ urls: turnServerURL, username: turnUserName, credential: turnCredential }],
        "bundlePolicy": 'max-bundle',
        "configuration": this.mediaConfiguration
    };
    peerconnection = null;
    closePeerConnection() {
        this.peerconnection.close();
        this.peerconnection = null;
    }
    setRemoteSDPAndCreateAnswer(sdp) {
        this.peerconnection.setRemoteDescription(new RTCSessionDescription(sdp)).then(
            () => {
                console.log("[info]--VIDEO-- remote sdp set");
            }
        );
        this.peerconnection.createAnswer((answer) => {
            this.peerconnection.setLocalDescription(answer).then(() => {
                console.log("[info]--VIDEO-- local answer set");
            });
        }, () => { }, this.mediaConfiguration);
    }

    establishPeerConnection(sendAnswerCallback, onTrackCallback) {

        this.peerconnection = new RTCPeerConnection(this.iceConfiguration);
        this.peerconnection.onicecandidate = (evt) => {
            console.log("[info]--VIDEO-- on ice candidate", evt);
        }
        this.peerconnection.ontrack = (event) => {
            console.log("[info]--VIDEO-- on track");
            onTrackCallback(event);
        };

        this.peerconnection.onicegatheringstatechange = (evt) => {
            console.log("[info]--VIDEO-- ice gathering state change");
            if (evt.target.iceGatheringState === 'complete') {
                console.log("[info]--VIDEO-- ice complete ");
                sendAnswerCallback(this.peerconnection.localDescription);
            }
        }
    }

    constructor() {
    }
};
class CustomServicePeerConnection {
    mediaConfiguration = {
        "OfferToReceiveAudio": true,
        "OfferToReceiveVideo": false
    };
    iceConfiguration = {
        "iceServers": [{ urls: turnServerURL, username: turnUserName, credential: turnCredential }],
        "bundlePolicy": 'max-bundle',
        "configuration": this.mediaConfiguration
    };
    getUserMediaConstraints = {
        audio: true,
        video: false
    };
    peerconnection = null;
    async setRemoteSDPAndCreateAnswer(sdp) {
        this.peerconnection.setRemoteDescription(new RTCSessionDescription(sdp)).then(
            () => {
                console.log("[info]--AUDIO-- remote sdp set");
            }
        );
        await navigator.mediaDevices.getUserMedia(this.getUserMediaConstraints).then(
            (audioStream) => {
                console.log("[info]--AUDIO-- get user media");
                this.peerconnection.addStream(audioStream);
                this.peerconnection.setLocalDescription(this.peerconnection.localDescription);
            }
        );
    }
    closePeerConnection() {
        this.peerconnection.close();
        this.peerconnection = null;
    }
    establishPeerConnection(sendAudioAnswerCallback, audioOnTrackCallback) {
        this.peerconnection.ontrack = (event) => {
            console.log("[info]--AUDIO-- on track");
            audioOnTrackCallback(event);
        };
        this.peerconnection.onicegatheringstatechange = (evt) => {
            console.log("[info]--AUDIO-- ice gathering state change");
            if (evt.target.iceGatheringState === 'complete') {
                console.log("[info]--AUDIO-- ICE COMPLETE");
                sendAudioAnswerCallback(this.peerconnection.localDescription);
            }
        }
    }

    constructor() {
        this.peerconnection = new RTCPeerConnection(this.iceConfiguration);
    }
};