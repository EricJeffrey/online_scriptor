
#include "uWebSockets/App.h"
#include <iostream>
#include <string>
#include <thread>
using namespace std;

struct PerSockData {
    string wskey;
    uWS::WebSocket<false, true, PerSockData> *ws;
};

PerSockData *wsData;

int main(int argc, char const *argv[]) {
    std::thread([]() {
        while (true) {
            if (wsData == nullptr)
                sleep(1);
            else {
                cout << "thread: sending hello to websocket" << endl;
                for (int i = 0; i < 10; i++) {
                    if (wsData != nullptr) {
                        wsData->ws->send("hello from uwebsocket", uWS::TEXT);
                        sleep(1);
                    } else
                        break;
                }
                cout << "done" << endl;
            }
        }
    }).detach();
    uWS::App()
        .get("/",
             [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
                 cout << "url: " << req->getUrl() << endl;
                 res->writeStatus(uWS::HTTP_200_OK);
                 res->end("<html><strong>Hello World</strong></html>");
             })
        .ws<PerSockData>(
            "/*", uWS::App::WebSocketBehavior<PerSockData>{
                      .idleTimeout = 16,
                      .sendPingsAutomatically = true,
                      .upgrade =
                          [](uWS::HttpResponse<false> *res, auto *req, auto *context) {
                              res->upgrade<PerSockData>(
                                  PerSockData{.wskey = string(req->getHeader("sec-websocket-key"))},
                                  req->getHeader("sec-websocket-key"),
                                  req->getHeader("sec-websocket-protocol"),
                                  req->getHeader("sec-websocket-extensions"), context);
                              req->getHeader("sec-websocket-key");
                          },
                      .open =
                          [](uWS::WebSocket<false, true, PerSockData> *ws) {
                              wsData = ws->getUserData();
                              wsData->ws = ws;
                          },
                      .message =
                          [](uWS::WebSocket<false, true, PerSockData> *ws, string_view message,
                             uWS::OpCode opcode) {
                              cout << "Ws Message, code: " << opcode << ", message: " << message
                                   << endl;
                          },
                      .close =
                          [](uWS::WebSocket<false, true, PerSockData> *ws, int code,
                             std::string_view message) {
                              /* You may access ws->getUserData() here */
                              wsData = nullptr;
                          }})
        .listen(8000,
                [](us_listen_socket_t *listenSock) {
                    if (listenSock) {
                        cout << "listening for 8000" << endl;
                    }
                })
        .run();
    return 0;
}
