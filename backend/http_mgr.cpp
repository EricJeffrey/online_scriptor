#include "http_mgr.hpp"
#include "buffer_helper.hpp"
#include "cmd_msg.hpp"
#include "config.hpp"
#include "util.hpp"

#include "fmt/format.h"
#include "uWebSockets/App.h"

#include <fstream>
#include <istream>
#include <optional>
#include <string>
#include <thread>

using std::string, std::optional, std::stoi;
using std::string_view, std::thread;
using uWS::HttpRequest;
using HttpResponseNoSSL = uWS::HttpResponse<false>;

constexpr string_view HTTP_200 = "200 Ok";
constexpr string_view HTTP_400 = "400 BadRequest";
constexpr string_view HTTP_404 = "404 NotFound";

constexpr string_view STATIC_FILE_PREFIX = "/static/";
constexpr string_view STATIC_FILE_DIR_PATH = "/data/online_scriptor_sjf/static/";

constexpr char HTTP_KEY_TASK_ID[] = "taskId";
constexpr char HTTP_KEY_TITLE[] = "title";
constexpr char HTTP_KEY_SCRIPT_TYPE[] = "scriptType";
constexpr char HTTP_KEY_SCRIPT_CODE[] = "scriptCode";
constexpr char HTTP_KEY_INTERVAL[] = "interval";
constexpr char HTTP_KEY_MAX_TIMES[] = "maxTimes";
constexpr char HTTP_KEY_STDIN_CONTENT[] = "stdinContent";

map<int32_t, PerWsData *> HttpMgr::taskId2WsData;
int HttpMgr::mIOSock = 0;
int HttpMgr::mCmdSock = 0;

CmdRes awaitTaskMgrRes(int fd, const CmdMsg &msg) {
    BufferHelper::writeOne(fd, msg.toJson());
    return CmdRes::parse(BufferHelper::readOne(fd));
}

void writeHttpResp(HttpResponseNoSSL *res, const CmdRes &cmdRes, string_view status = HTTP_200) {
    res->writeStatus(status);
    res->end(cmdRes.toJsonStr());
}

// find taskId in query paramter, write HTTP_400 if not found
optional<int> checkTaskId(HttpResponseNoSSL *res, HttpRequest *req) {
    string_view taskIdParam = req->getQuery(HTTP_KEY_TASK_ID);
    try {
        if (!taskIdParam.empty())
            return stoi(string(taskIdParam));
    } catch (std::exception &) {
    }
    return std::nullopt;
}

void HttpMgr::start(int cmdSock, int ioSock) {
    HttpMgr::mCmdSock = cmdSock;
    HttpMgr::mIOSock = ioSock;

    auto wsThread = thread(wsIODataThread, ioSock);

    auto getTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        auto taskIdOpt = checkTaskId(res, req);
        if (taskIdOpt.has_value()) {
            writeHttpResp(res,
                          awaitTaskMgrRes(HttpMgr::mCmdSock, CmdMsg{.cmdType = CmdMsg::GET_TASK,
                                                                    .taskId = taskIdOpt.value()}));
        }
    };

    auto getAllTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        CmdRes cmdRes = awaitTaskMgrRes(HttpMgr::mCmdSock, CmdMsg{.cmdType = CmdMsg::GET_ALL_TASK});
        writeHttpResp(res, cmdRes);
    };

    auto createTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        string *bufPtr = new string();
        res->onData([res, bufPtr](string_view data, bool isLast) {
            bufPtr->append(data);
            if (isLast) {
                json reqBody = json::parse(*bufPtr);
                CmdRes cmdRes{.status = CmdRes::FAILED};
                try {
                    cmdRes = awaitTaskMgrRes(
                        HttpMgr::mCmdSock,
                        CmdMsg{
                            .cmdType = CmdMsg::CREATE_TASK,
                            .title = reqBody[HTTP_KEY_TITLE].get<string>(),
                            .scriptCode = reqBody[HTTP_KEY_SCRIPT_CODE].get<string>(),
                            .scriptType = reqBody[HTTP_KEY_SCRIPT_TYPE].get<TaskScriptType>(),
                            .interval = reqBody[HTTP_KEY_INTERVAL].get<int32_t>(),
                            .maxTimes = reqBody[HTTP_KEY_MAX_TIMES].get<int32_t>(),
                        });
                    writeHttpResp(res, cmdRes);
                } catch (const json::exception &e) {
                    writeHttpResp(res, cmdRes, HTTP_400);
                }
                delete bufPtr;
            }
        });
    };

    auto startTask = [](HttpResponseNoSSL *res, HttpRequest *req) {

    };

    auto stopTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        auto taskIdOpt = checkTaskId(res, req);
        if (taskIdOpt.has_value()) {
            res->end(awaitTaskMgrRes(HttpMgr::mCmdSock,
                                     CmdMsg{
                                         .cmdType = CmdMsg::STOP_TASK,
                                         .taskId = taskIdOpt.value(),
                                     })
                         .toJsonStr());
        }
    };

    auto deleteTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        auto taskIdOpt = checkTaskId(res, req);
        if (taskIdOpt.has_value()) {
            res->end(awaitTaskMgrRes(HttpMgr::mCmdSock,
                                     CmdMsg{
                                         .cmdType = CmdMsg::DELETE_TASK,
                                         .taskId = taskIdOpt.value(),
                                     })
                         .toJsonStr());
        }
    };

    auto enableIORedirect = [](HttpResponseNoSSL *res, HttpRequest *req) {
        auto taskIdOpt = checkTaskId(res, req);
        if (taskIdOpt.has_value()) {
            res->end(awaitTaskMgrRes(HttpMgr::mCmdSock,
                                     CmdMsg{
                                         .cmdType = CmdMsg::ENABLE_REDIRECT,
                                         .taskId = taskIdOpt.value(),
                                     })
                         .toJsonStr());
        }
    };

    auto disableIORedirect = [](HttpResponseNoSSL *res, HttpRequest *req) {
        auto taskIdOpt = checkTaskId(res, req);
        if (taskIdOpt.has_value()) {
            res->end(awaitTaskMgrRes(HttpMgr::mCmdSock,
                                     CmdMsg{
                                         .cmdType = CmdMsg::DISABLE_REDIRECT,
                                         .taskId = taskIdOpt.value(),
                                     })
                         .toJsonStr());
        }
    };

    auto putToStdin = [](HttpResponseNoSSL *res, HttpRequest *req) {
        string *bufPtr = new string();
        res->onData([res, bufPtr](string_view data, bool isLast) {
            bufPtr->append(data);
            if (isLast) {
                json reqBody = json::parse(*bufPtr);
                CmdRes cmdRes{.status = CmdRes::FAILED};
                try {
                    cmdRes = awaitTaskMgrRes(
                        HttpMgr::mCmdSock,
                        CmdMsg{
                            .cmdType = CmdMsg::PUT_TO_STDIN,
                            .taskId = reqBody[HTTP_KEY_TASK_ID].get<int32_t>(),
                            .stdinContent = reqBody[HTTP_KEY_STDIN_CONTENT].get<string>(),
                        });
                    writeHttpResp(res, cmdRes);
                } catch (const json::exception &e) {
                    writeHttpResp(res, cmdRes, HTTP_400);
                }
                delete bufPtr;
            }
        });
    };

    auto serveFile = [](HttpResponseNoSSL *res, HttpRequest *req) {
        const string fpath =
            string(STATIC_FILE_DIR_PATH) + string(req->getUrl().substr(STATIC_FILE_PREFIX.size()));
        if (filePathOk(fpath)) {
            res->writeStatus(HTTP_200);
            constexpr int bufsz = 4096;
            char buf[bufsz];
            string data;
            FILE *file = fopen(fpath.c_str(), "rt");
            while (!feof(file)) {
                size_t n = fread(buf, 1, bufsz, file);
                data.append(buf, n);
            }
            fclose(file);
            res->end(data);
        } else {
            res->writeStatus(HTTP_404);
            res->writeHeader("Content-Type", "text/html");
            res->end("<strong style=\"font-size:x-large;\">404</strong>");
        }
    };

    uWS::App()
        .get("/static/*", serveFile)
        .get("/task", getTask)
        .get("/task/all", getAllTask)
        .get("/task/start", startTask)
        .get("/task/stop", stopTask)
        .get("/task/delete", deleteTask)
        .get("/task/ioredirect/disable", enableIORedirect)
        .get("/task/ioredirect/enable", disableIORedirect)
        .post("/task/create", createTask)
        .post("/task/puttostdin", putToStdin)
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
        .listen(PORT,
                [](us_listen_socket_t *listenSock) {
                    if (listenSock)
                        fmt::print("http server started, listening on port {}\n", PORT);
                })
        .run();
    fmt::print("HttpMgr: http server exited, waiting websocket io data Thread\n");
    close(HttpMgr::mIOSock), close(HttpMgr::mCmdSock);
    wsThread.join();
}