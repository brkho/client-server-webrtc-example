// No-op implementations of most webrtc::*Observer methods. For the ones we do care about in the
// example, we supply a callback in the constructor.
//
// Author: brian@brkho.com

#ifndef WEBRTC_EXAMPLE_SERVER_OBSERVERS_H
#define WEBRTC_EXAMPLE_SERVER_OBSERVERS_H

#include <webrtc/api/peerconnectioninterface.h>

#include <functional>

// PeerConnection events.
class PeerConnectionObserver : public webrtc::PeerConnectionObserver {
  public:
    // Constructor taking a few callbacks.
    PeerConnectionObserver(std::function<void(webrtc::DataChannelInterface*)> on_data_channel,
        std::function<void(const webrtc::IceCandidateInterface*)> on_ice_candidate) :
        on_data_channel{on_data_channel}, on_ice_candidate{on_ice_candidate} {}

    // Override signaling change.
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState /* new_state */) {}

    // Override adding a stream.
    void OnAddStream(webrtc::MediaStreamInterface* /* stream */) {}

    // Override removing a stream.
    void OnRemoveStream(webrtc::MediaStreamInterface* /* stream */) {}

    // Override data channel change.
    void OnDataChannel(webrtc::DataChannelInterface* channel) {
      on_data_channel(channel);
    }

    // Override renegotiation.
    void OnRenegotiationNeeded() {}

    // Override ICE connection change.
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState /* new_state */) {}

    // Override ICE gathering change.
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState /* new_state */) {}

    // Override ICE candidate.
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
      on_ice_candidate(candidate);
    }

  private:
    std::function<void(webrtc::DataChannelInterface*)> on_data_channel;
    std::function<void(const webrtc::IceCandidateInterface*)> on_ice_candidate;
};

// DataChannel events.
class DataChannelObserver : public webrtc::DataChannelObserver {
  public:
    // Constructor taking a callback.
    DataChannelObserver(std::function<void(const webrtc::DataBuffer&)> on_message) :
        on_message{on_message} {}

    // Change in state of the Data Channel.
    void OnStateChange() {}
    
    // Message received.
    void OnMessage(const webrtc::DataBuffer& buffer) {
      on_message(buffer);
    }

    // Buffered amount change.
    void OnBufferedAmountChange(uint64_t /* previous_amount */) {}

  private:
    std::function<void(const webrtc::DataBuffer&)> on_message;
};

// Create SessionDescription events.
class CreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
  public:
    // Constructor taking a callback.
    CreateSessionDescriptionObserver(std::function<void(webrtc::SessionDescriptionInterface*)>
        on_success) : on_success{on_success} {}
  
    // Successfully created a session description.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
      on_success(desc);
    }

    // Failure to create a session description.
    void OnFailure(const std::string& /* error */) {}

    // Unimplemented virtual function.
    int AddRef() const { return 0; }

    // Unimplemented virtual function.
    int Release() const { return 0; }

  private:
    std::function<void(webrtc::SessionDescriptionInterface*)> on_success;
};

// Set SessionDescription events.
class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
  public:
    // Default constructor.
    SetSessionDescriptionObserver() {}

    // Successfully set a session description.
    void OnSuccess() {}

    // Failure to set a sesion description.
    void OnFailure(const std::string& /* error */) {}

    // Unimplemented virtual function.
    int AddRef() const { return 0; }

    // Unimplemented virtual function.
    int Release() const { return 0; }
};

#endif  // WEBRTC_EXAMPLE_SERVER_OBSERVERS_H
