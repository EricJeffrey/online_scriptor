#include "http_mgr.hpp"

#include "uWebSockets/App.h"
#include <thread>

using std::string_view, std::thread;

const int32_t HttpMgr::PORT = 8000;
map<int32_t, PerWsData *> HttpMgr::taskId2WsData;
int HttpMgr::mIOSock = 0;
int HttpMgr::mCmdSock = 0;

void wsIODataThread(int ioSock) {
    // TODO read io data and send to ws here
}

void HttpMgr::start(int cmdSock, int ioSock) {
    HttpMgr::mCmdSock = cmdSock;
    HttpMgr::mIOSock = ioSock;

    auto wsThread = thread(wsIODataThread, ioSock);

    // TODO make http server
    uWS::App()
        .ws<PerWsData>(
            "/ws", uWS::App::WebSocketBehavior<PerWsData>{
                       .idleTimeout = 16,
                       .sendPingsAutomatically = true,
                       .upgrade =
                           [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, auto *context) {
                               res->upgrade<PerWsData>(
                                   PerWsData{
                                       .taskId = std::stoi(req->getParameter(0).data()),
                                   },
                                   req->getHeader("sec-websocket-key"),
                                   req->getHeader("sec-websocket-protocol"),
                                   req->getHeader("sec-websocket-extensions"), context);
                           },
                       .open =
                           [](uWS::WebSocket<false, true, PerWsData> *ws) {
                               ws->getUserData()->ws = ws;
                               HttpMgr::taskId2WsData[ws->getUserData()->taskId] =
                                   ws->getUserData();
                           },
                       .message = [](uWS::WebSocket<false, true, PerWsData> *ws,
                                     string_view message, uWS::OpCode opcode) {},
                       .close =
                           [](uWS::WebSocket<false, true, PerWsData> *ws, int code,
                              std::string_view message) {
                               HttpMgr::taskId2WsData.erase(ws->getUserData()->taskId);
                           },
                   })
        .listen(HttpMgr::PORT,
                [](us_listen_socket_t *listenSock) {
                    if (listenSock)
                        printf("http server started, listening on port %d\n", PORT);
                })
        .run();
    printf("HttpMgr: http server exited, waiting websocket io data Thread\n");
    close(HttpMgr::mIOSock), close(HttpMgr::mCmdSock);
    wsThread.join();
}