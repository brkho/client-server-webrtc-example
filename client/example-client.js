/**
 * This a minimal fully functional example for setting up a client written in JavaScript that
 * communicates with a server via WebRTC data channels. This uses WebSockets to perform the WebRTC
 * handshake (offer/accept SDP) with the server. We only use WebSockets for the initial handshake
 * because TCP often presents too much latency in the context of real-time action games. WebRTC
 * data channels, on the other hand, allow for unreliable and unordered message sending via SCTP
 *
 * Brian Ho
 * brian@brkho.com
 */

// URL to the server with the port we are using for WebSockets.
const webSocketUrl = 'ws://<server IP address>:8080';
// The WebSocket object used to manage a connection.
let webSocketConnection = null;
// The RTCPeerConnection through which we engage in the SDP handshake.
let rtcPeerConnection = null;
// The data channel used to communicate.
let dataChannel = null;

// Callback for when we receive a message on the data channel.
function onDataChannelMessage(event) {
  console.log(event.data);
}

// Callback for when the data channel was successfully opened.
function onDataChannelOpen() {
  console.log('Data channel opened!');
}

// Callback for when the STUN server responds with the ICE candidates.
function onIceCandidate(event) {
  if (event && event.candidate) {
    webSocketConnection.send(JSON.stringify({type: 'candidate', payload: event.candidate}));
  }
}

// Callback for when the SDP offer was successfully created.
function onOfferCreated(description) {
  rtcPeerConnection.setLocalDescription(description);
  webSocketConnection.send(JSON.stringify({type: 'offer', payload: description}));
}

// Callback for when the WebSocket is successfully opened.
function onWebSocketOpen() {
  const config = { iceServers: [{ url: 'stun:stun.l.google.com:19302' }] };
  rtcPeerConnection = new RTCPeerConnection(config);
  const dataChannelConfig = { ordered: false, maxRetransmits: 0 };
  dataChannel = rtcPeerConnection.createDataChannel('dc', dataChannelConfig);
  dataChannel.onmessage = onDataChannelMessage;
  dataChannel.onopen = onDataChannelOpen;
  const sdpConstraints = {
    mandatory: {
      OfferToReceiveAudio: false,
      OfferToReceiveVideo: false,
    },
  };
  rtcPeerConnection.onicecandidate = onIceCandidate;
  rtcPeerConnection.createOffer(onOfferCreated, () => {}, sdpConstraints);
}

// Callback for when we receive a message from the server via the WebSocket.
function onWebSocketMessage(event) {
  const messageObject = JSON.parse(event.data);
  if (messageObject.type === 'answer') {
    rtcPeerConnection.setRemoteDescription(new RTCSessionDescription(messageObject.payload));
  } else if (messageObject.type === 'candidate') {
    rtcPeerConnection.addIceCandidate(new RTCIceCandidate(messageObject.payload));
  } else {
    console.log('Unrecognized WebSocket message type.');
  }
}

// Connects by creating a new WebSocket connection and associating some callbacks.
function connect() {
  webSocketConnection = new WebSocket(webSocketUrl);
  webSocketConnection.onopen = onWebSocketOpen;
  webSocketConnection.onmessage = onWebSocketMessage;
}

// Pings the server via the DataChannel once the connection has been established.
function ping() {
  dataChannel.send('ping');
}
