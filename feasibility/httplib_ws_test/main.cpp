
#include "websocketpp/config/asio.hpp"
#include "websocketpp/server.hpp"

using WsServer = websocketpp::server<websocketpp::config::asio>;
using std::bind;
using std::string;
using std::placeholders::_1, std::placeholders::_2;

void onHttp(WsServer *s, websocketpp::connection_hdl hdl) {
    auto conn = s->get_con_from_hdl(hdl);
    const auto &req = conn->get_request();
    const string &path = req.get_uri();
    const string &head1 = req.get_header("User-Agent");
    const string &body = req.get_body();

    conn->set_body(path + "\n" + head1 + "\n" + body);
    conn->set_status(websocketpp::http::status_code::ok);
}

int main(int argc, char const *argv[]) {
    WsServer server;
    server.set_access_channels(websocketpp::log::alevel::all);
    server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    server.init_asio();
    server.set_reuse_addr(true);

    server.set_http_handler(bind(&onHttp, &server, _1));
    server.listen(8000);
    server.start_accept();
    server.run();
    return 0;
}
