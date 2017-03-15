// This a minimal fully functional example for setting up a server written in C++ that communicates
// with clients via WebRTC data channels. This uses WebSockets to perform the WebRTC handshake
// (offer/accept SDP) with the client. We only use WebSockets for the initial handshake because TCP
// often presents too much latency in the context of real-time action games. WebRTC data channels,
// on the other hand, allow for unreliable and unordered message sending via SCTP.
//
// Author: brian@brkho.com

#include "observers.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/base/physicalsocketserver.h>
#include <webrtc/base/ssladapter.h>
#include <webrtc/base/thread.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <thread>

// WebSocket++ types are gnarly.
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// Some forward declarations.
void OnDataChannelCreated(webrtc::DataChannelInterface* channel);
void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
void OnDataChannelMessage(const webrtc::DataBuffer& buffer);
void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc);

typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
typedef WebSocketServer::message_ptr message_ptr;

// The WebSocket server being used to handshake with the clients.
WebSocketServer ws_server;
// The peer conncetion factory that sets up signaling and worker threads. It is also used to create
// the PeerConnection.
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
// The socket that the signaling thread and worker thread communicate on.
rtc::PhysicalSocketServer socket_server;
// The separate thread where all of the WebRTC code runs since we use the main thread for the
// WebSocket listening loop.
std::thread webrtc_thread;
// The WebSocket connection handler that uniquely identifies one of the connections that the
// WebSocket has open. If you want to have multiple connections, you will need to store more than
// one of these.
websocketpp::connection_hdl websocket_connection_handler;
// The peer connection through which we engage in the SDP handshake.
rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
// The data channel used to communicate.
rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
// The observer that responds to peer connection events.
PeerConnectionObserver peer_connection_observer(OnDataChannelCreated, OnIceCandidate);
// The observer that responds to data channel events.
DataChannelObserver data_channel_observer(OnDataChannelMessage);
// The observer that responds to session description creation events.
CreateSessionDescriptionObserver create_session_description_observer(OnAnswerCreated);
// The observer that responds to session description set events. We don't really use this one here.
SetSessionDescriptionObserver set_session_description_observer;


// Callback for when the data channel is successfully created. We need to re-register the updated
// data channel here.
void OnDataChannelCreated(webrtc::DataChannelInterface* channel) {
  data_channel = channel;
  data_channel->RegisterObserver(&data_channel_observer);
}

// Callback for when the STUN server responds with the ICE candidates.
void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  std::string candidate_str;
  candidate->ToString(&candidate_str);
  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember("type", "candidate", message_object.GetAllocator());
  rapidjson::Value candidate_value;
  candidate_value.SetString(rapidjson::StringRef(candidate_str.c_str()));
  rapidjson::Value sdp_mid_value;
  sdp_mid_value.SetString(rapidjson::StringRef(candidate->sdp_mid().c_str()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember("candidate", candidate_value, message_object.GetAllocator());
  message_payload.AddMember("sdpMid", sdp_mid_value, message_object.GetAllocator());
  message_payload.AddMember("sdpMLineIndex", candidate->sdp_mline_index(),
      message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  message_object.Accept(writer);
  std::string payload = strbuf.GetString();
  ws_server.send(websocket_connection_handler, payload, websocketpp::frame::opcode::value::text);
}

// Callback for when the server receives a message on the data channel.
void OnDataChannelMessage(const webrtc::DataBuffer& buffer) {
  // std::string data(buffer.data.data<char>(), buffer.data.size());
  // std::cout << data << std::endl;
  // std::string str = "pong";
  // webrtc::DataBuffer resp(rtc::CopyOnWriteBuffer(str.c_str(), str.length()), false /* binary */);
  data_channel->Send(buffer);
}

// Callback for when the answer is created. This sends the answer back to the client.
void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc) {
  peer_connection->SetLocalDescription(&set_session_description_observer, desc);
  // Apologies for the poor code ergonomics here; I think rapidjson is just verbose.
  std::string offer_string;
  desc->ToString(&offer_string);
  rapidjson::Document message_object;
  message_object.SetObject();
  message_object.AddMember("type", "answer", message_object.GetAllocator());
  rapidjson::Value sdp_value;
  sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));
  rapidjson::Value message_payload;
  message_payload.SetObject();
  message_payload.AddMember("type", "answer", message_object.GetAllocator());
  message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
  message_object.AddMember("payload", message_payload, message_object.GetAllocator());
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  message_object.Accept(writer);
  std::string payload = strbuf.GetString();
  ws_server.send(websocket_connection_handler, payload, websocketpp::frame::opcode::value::text);
}

// Callback for when the WebSocket server receives a message from the client.
void OnWebSocketMessage(WebSocketServer* /* s */, websocketpp::connection_hdl hdl, message_ptr msg) {
  websocket_connection_handler = hdl;
  rapidjson::Document message_object;
  message_object.Parse(msg->get_payload().c_str());
  // Probably should do some error checking on the JSON object.
  std::string type = message_object["type"].GetString();
  if (type == "ping") {
    std::string id = msg->get_payload().c_str();
    ws_server.send(websocket_connection_handler, id, websocketpp::frame::opcode::value::text);
  } else if (type == "offer") {
    std::string sdp = message_object["payload"]["sdp"].GetString();
    webrtc::PeerConnectionInterface::RTCConfiguration configuration;
    webrtc::PeerConnectionInterface::IceServer ice_server;
    ice_server.uri = "stun:stun.l.google.com:19302";
    configuration.servers.push_back(ice_server);

    peer_connection = peer_connection_factory->CreatePeerConnection(configuration, nullptr, nullptr,
        &peer_connection_observer);
    webrtc::DataChannelInit data_channel_config;
    data_channel_config.ordered = false;
    data_channel_config.maxRetransmits = 0;
    data_channel = peer_connection->CreateDataChannel("dc", &data_channel_config);
    data_channel->RegisterObserver(&data_channel_observer);

    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription("offer", sdp, &error));
    peer_connection->SetRemoteDescription(&set_session_description_observer, session_description);
    peer_connection->CreateAnswer(&create_session_description_observer, nullptr);
  } else if (type == "candidate") {
    std::string candidate = message_object["payload"]["candidate"].GetString();
    int sdp_mline_index = message_object["payload"]["sdpMLineIndex"].GetInt();
    std::string sdp_mid = message_object["payload"]["sdpMid"].GetString();
    webrtc::SdpParseError error;
    auto candidate_object = webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error);
    peer_connection->AddIceCandidate(candidate_object);
  } else {
    std::cout << "Unrecognized WebSocket message type." << std::endl;
  }
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as the signaling thread
// and creates a worker thread in the background.
void SignalThreadEntry() {
  // Create the PeerConnectionFactory.
  rtc::InitializeSSL();
  peer_connection_factory = webrtc::CreatePeerConnectionFactory();
  rtc::Thread* signaling_thread = rtc::Thread::Current();
  signaling_thread->set_socketserver(&socket_server);
  signaling_thread->Run();
  signaling_thread->set_socketserver(nullptr);
}

// Main entry point of the code.
int main() {
  webrtc_thread = std::thread(SignalThreadEntry);
  // In a real game server, you would run the WebSocket server as a separate thread so your main
  // process can handle the game loop.
  ws_server.set_message_handler(bind(OnWebSocketMessage, &ws_server, ::_1, ::_2));
  ws_server.init_asio();
  ws_server.clear_access_channels(websocketpp::log::alevel::all);
  ws_server.set_reuse_addr(true);
  ws_server.listen(8080);
  ws_server.start_accept();
  // I don't do it here, but you should gracefully handle closing the connection.
  ws_server.run();
  rtc::CleanupSSL();
}
