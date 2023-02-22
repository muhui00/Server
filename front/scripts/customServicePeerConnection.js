var turnServerURL = "stun:173.255.200.200:3478";
var turnUserName = "";
var turnCredential = "";
window.AudioContext = window.AudioContext || window.webkitAudioContext;
var RTCPeerConnection = window.RTCPeerConnection || window.mozRTCPeerConnection || window.webkitRTCPeerConnection;
var RTCSessionDescription = window.mozRTCSessionDescription || window.webkitRTCSessionDescription || window.RTCSessionDescription;
var websocketConnectionURL = "ws://127.0.0.1:1234/customService/connect";
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
    connectionId = -1;
    requestId = -1;
    remoteVideo = null;
    remoteAudio = null;
    getRequestListBtn = null;
    stopCustomServiceBtn = null;
    answeringState = false;
    customServiceRequestList = null;
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
                    case "getRequestListBtn":
                        pair[1].onclick = () => {
                            this.sendJsonMessage({
                                "type": "getRequestList"
                            });
                        };
                        break;
                };
            }
        );
    }
    requestListItemCreator = (item) => {
        return "<li>requestId:{0}; estateId:{1}; <button id='request-btn-{0}'>accept</button></li>".format(item.requestId, item.estateId);
    };
    sendJsonMessage(jsonValue) {
        jsonValue["from"] = "customService";
        console.log("[send message] send to server: ", jsonValue);
        this.webSocketConnection.send(JSON.stringify(jsonValue));
    }
    getCustomServiceRequestList() {
        this.sendJsonMessage(
            {
                "type": "getRequestList"
            }
        );
    }

    stopCustomService(isInitiative) {
        this.remoteVideo.style.visibility = 'hidden';
        this.getRequestListBtn.disabled = false;
        if (this.videoPeerConnection)
            this.videoPeerConnection.closePeerConnection();
        if (this.customServicePeerConnection)
            this.customServicePeerConnection.closePeerConnection();
        this.videoPeerConnection = null;
        this.customServicePeerConnection = null;

        if (isInitiative) {
            let jsonMessage = {
                "type": "serviceStuffStopCustomService",
                "connectionId": this.connectionId,
                "requestId": this.requestId
            };
            this.sendJsonMessage(jsonMessage);
        }
        this.requestId = -1;
        this.connectionId = -1;
    }


    handleMessageFromReceiver(jsonMessage) {
        switch (jsonMessage.type) {
            case "answer":
                this.customServicePeerConnection.setRemoteSDP(jsonMessage.sdp);
                break;
            case "userCloseVideoConnection":
                this.stopCustomService(false);
                break;
        }
    }
    handleMessageFromSender(jsonMessage) {
        let _sendAnswerToSenderCallback = (answer) => {
            this.sendJsonMessage({
                type: "answer",
                data: answer,
                "to": "sender",
                "requestId": this.requestId,
            });
        };
        let _videoConnectionOnTrackCallback = (trackEvent) => {
            console.log("[info]--VIDEO-- On Track");
            this.remoteVideo.srcObject = trackEvent.streams[0];
            this.remoteVideo.play();
            this.remoteVideo.style.visibility = 'visible';
        };

        switch (jsonMessage.type) {
            case "offer":
                this.videoPeerConnection = new VideoDisplayPeerConnection();
                this.videoPeerConnection.establishPeerConnection(_sendAnswerToSenderCallback, _videoConnectionOnTrackCallback);
                this.videoPeerConnection.setRemoteSDPAndCreateAnswer(jsonMessage.sdp);
                break;
            case "closeVideoConnection":
                this.stopCustomService(false);
        }
    }
    handleErrorMessage(jsonMessage) {

    }
    acceptRequest(requestId) {
        this.sendJsonMessage({
            type: "acceptCustomServiceRequest",
            "requestId": requestId
        }
        );
        this.answeringState = true;
    }
    constructor() {
        let _webSocketOnMessageCallback = (evt) => {
            if (evt.data === '' || evt.data === '\x00\x00\x00\x00') {
                return;
            }
            let jsonMessage = JSON.parse(evt.data);
            console.log("[on message]", jsonMessage);
            if (("from" in jsonMessage) == true) {
                if (jsonMessage.from === "receiver") {
                    this.handleMessageFromReceiver(jsonMessage);
                }
                else if (jsonMessage.from === "sender") {
                    this.handleMessageFromSender(jsonMessage);
                }
            }
            else {
                switch (jsonMessage.type) {
                    case "error":
                        window.alert(jsonMessage.errorMsg);
                        break;
                    case "customServiceRequestList":
                        this.customServiceRequestList.innerHTML = "";
                        if (jsonMessage.data == null) {
                            return;
                        }
                        jsonMessage.data.forEach(
                            (elem) => {
                                this.customServiceRequestList.innerHTML +=
                                    this.requestListItemCreator(elem);
                            }
                        );
                        // add [onclick] event handler for button
                        jsonMessage.data.forEach(
                            (elem) => {
                                document.getElementById("request-btn-{0}".format(elem.requestId)).onclick =
                                    () => {
                                        this.acceptRequest(elem.requestId);
                                    };
                            }
                        );
                        break;
                    case "customServiceAcceptRequestSucceed":
                        //this.connectionId = jsonMessage.connectionId;
                        this.requestId = jsonMessage.requestId;
                        let _sendAudioOfferCallback = (audioOffer) => {
                            this.sendJsonMessage(
                                {
                                    "requestId": this.requestId,
                                    "connectionId": this.connectionId,
                                    "to": "receiver",
                                    "type": "offer",
                                    "sdp": audioOffer
                                }
                            );
                        };
                        this.customServicePeerConnection = new CustomServicePeerConnection();
                        this.customServicePeerConnection.establishPeerConnection(_sendAudioOfferCallback);
                        break;
                    case "customServiceRequestInvalidation":
                        this.answeringState = false;
                        window.alert("invalidation of request!");
                        break;
                    case "userStopCustomServiceRequest":
                        this.answeringState = false;
                        this.stopCustomService(false);
                        break;

                }
            }
        };
        this.webSocketConnection = new WebSocket(websocketConnectionURL);
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
                console.log("[info]--VIDEO-- ICE COMPLETE");
                sendAnswerCallback(this.peerconnection.localDescription);
            }
        }
    }
    closePeerConnection() {
        this.peerconnection.close();
        this.peerconnection = null;
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
    setRemoteSDP(sdp) {
        this.peerconnection.setRemoteDescription(new RTCSessionDescription(sdp)).then(
            () => {
                console.log("[info]--AUDIO-- remote sdp set");
            }
        );
    }
    async establishPeerConnection(sendAudioOfferCallback) {
        await navigator.mediaDevices.getUserMedia(this.getUserMediaConstraints).then(
            (audioStream) => {
                console.log("[info]--AUDIO-- get user media");
                this.peerconnection.addStream(audioStream);
                this.peerconnection.setLocalDescription(this.peerconnection.localDescription);
            }
        );
        this.peerconnection.ontrack = (event) => {
            console.log("[info]--AUDIO-- on track");
            this.remoteAudio.srcObject = trackEvent.streams[0];
            his.remoteAudio.play();
        };
        this.peerconnection.onicegatheringstatechange = (evt) => {
            console.log("[info]--AUDIO-- ice gathering state change");
            if (evt.target.iceGatheringState === 'complete') {
                console.log("[info]--AUDIO-- ICE COMPLETE");
                sendAudioOfferCallback(this.peerconnection.localDescription);
            }
        }
    }
    closePeerConnection() {
        this.peerconnection.close();
        this.peerconnection = null;
    }
    constructor() {
        this.peerconnection = new RTCPeerConnection(this.iceConfiguration);
    }
};