
#include "uWebSockets/App.h"
#include <iostream>
#include <string>
using namespace std;

struct PerSockData {
    string wskey;
};

int main(int argc, char const *argv[]) {
    uWS::App()
        .get("/",
             [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
                 cout << "url: " << req->getUrl() << endl;
                 res->writeStatus(uWS::HTTP_200_OK);
                 res->end("<html><strong>Hello World</strong></html>");
             })
        // .ws<PerSockData>(
        //     "/*",
        //     uWS::App::WebSocketBehavior<PerSockData>{
        //         .idleTimeout = 16,
        //         .upgrade =
        //             [](uWS::HttpResponse<false> *res, auto *req, auto *context) {
        //                 res->upgrade<PerSockData>(
        //                     PerSockData{.wskey = string(req->getHeader("sec-websocket-key"))},
        //                     req->getHeader("sec-websocket-key"),
        //                     req->getHeader("sec-websocket-protocol"),
        //                     req->getHeader("sec-websocket-extensions"), context);
        //                 req->getHeader("sec-websocket-key");
        //             }})
        .listen(8000,
                [](us_listen_socket_t *listenSock) {
                    if (listenSock) {
                        cout << "listening for 8000" << endl;
                    }
                })
        .run();
    return 0;
}
