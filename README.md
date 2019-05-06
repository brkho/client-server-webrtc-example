## Client-Server WebRTC Example

This implements a minimal example with a client written in JavaScript that communicates with a server written in C++ using WebRTC over unordered and unreliable SCTP. While more involved to set up than WebSockets, this has the same advantage of lower latency that UDP has in real-time applications.

This repo is meant to accompany [my blog post on WebRTC web games](http://blog.brkho.com/2017/03/15/dive-into-client-server-web-games-webrtc/), so head there for explanation and setup instructions.

The example depends on [websocketpp](https://github.com/zaphoyd/websocketpp) and [rapidjson](https://github.com/miloyip/rapidjson).

You can contact me at brian@brkho.com.
