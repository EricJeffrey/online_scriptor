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

constexpr char HTTP_KEY_TASK_ID[] = "taskId";
constexpr char HTTP_KEY_TITLE[] = "title";
constexpr char HTTP_KEY_SCRIPT_TYPE[] = "scriptType";
constexpr char HTTP_KEY_SCRIPT_CODE[] = "scriptCode";
constexpr char HTTP_KEY_INTERVAL[] = "interval";
constexpr char HTTP_KEY_MAX_TIMES[] = "maxTimes";
constexpr char HTTP_KEY_STDIN_CONTENT[] = "stdinContent";

TaskWsHelper HttpMgr::taskWsHelper;
int HttpMgr::mIOSock = 0;
int HttpMgr::mCmdSock = 0;

// send msg to TaskMgr and wait for response
CmdRes awaitTaskMgrRes(int fd, const CmdMsg &msg) {
    BufferHelper::writeOne(fd, msg.toJson());
    return CmdRes::parse(BufferHelper::readOne(fd));
}

void writeHttpResp(HttpResponseNoSSL *res, const CmdRes &cmdRes, string_view status = HTTP_200) {
    res->cork([res, cmdRes, status]() { res->writeStatus(status)->end(cmdRes.toJsonStr()); });
}

// find taskId in query paramter, write HTTP_400 if not found or less than 1
optional<int32_t> checkTaskId(HttpResponseNoSSL *res, HttpRequest *req) {
    string_view taskIdParam = req->getQuery(HTTP_KEY_TASK_ID);
    try {
        if (!taskIdParam.empty()) {
            int32_t taskId = stoi(string(taskIdParam));
            if (taskId > 0)
                return taskId;
        }
    } catch (std::exception &e) {
        fmt::print("__DEBUG, checkTaskId failed: {}\n", e.what());
    }
    writeHttpResp(res, CmdRes{.status = CmdRes::FAILED}, HTTP_400);
    return std::nullopt;
}

void taskOpDelegate(HttpResponseNoSSL *res, HttpRequest *req, CmdMsg::Type cmdType) {
    auto taskIdOpt = checkTaskId(res, req);
    if (taskIdOpt.has_value()) {
        writeHttpResp(res, awaitTaskMgrRes(HttpMgr::mCmdSock, CmdMsg{.cmdType = cmdType,
                                                                     .taskId = taskIdOpt.value()}));
    }
}

// read file content and send to res, send 404 if file not found or not regular
void serveFile(const string &fpath, HttpResponseNoSSL *res) {
    if (filePathOk(fpath)) {
        res->writeStatus(HTTP_200);
        constexpr string_view contentType = "Content-Type";
        if (endsWith(fpath, ".html"))
            res->writeHeader(contentType, "text/html");
        if (endsWith(fpath, ".css"))
            res->writeHeader(contentType, "text/css");
        if (endsWith(fpath, ".js"))
            res->writeHeader(contentType, "text/javascript");
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
        res->end("Resource Not Found");
    }
}

void HttpMgr::start(int cmdSock, int ioSock) {
    HttpMgr::mCmdSock = cmdSock;
    HttpMgr::mIOSock = ioSock;

    auto wsThread = thread(wsIODataThread, ioSock);

    auto getTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: getTask\n");
        taskOpDelegate(res, req, CmdMsg::GET_TASK);
    };

    auto getAllTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: getAllTask\n");
        CmdRes cmdRes = awaitTaskMgrRes(HttpMgr::mCmdSock, CmdMsg{.cmdType = CmdMsg::GET_ALL_TASK});
        writeHttpResp(res, cmdRes);
    };

    auto createTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: createTask\n");
        string *bufPtr = new string();
        res->onData([res, bufPtr](string_view data, bool isLast) {
            bufPtr->append(data);
            if (isLast) {
                CmdRes cmdRes{.status = CmdRes::FAILED};
                try {
                    json reqBody = json::parse(*bufPtr);
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
                } catch (const json::exception &e) {
                    fmt::print("__DEBUG, createTask.onData, json exception: {}\n", e.what());
                }
                if (cmdRes.status == CmdRes::FAILED)
                    writeHttpResp(res, cmdRes, HTTP_400);
                else
                    writeHttpResp(res, cmdRes);
                delete bufPtr;
            }
        });
        res->onAborted([bufPtr]() { delete bufPtr; });
    };

    auto startTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: startTask\n");
        taskOpDelegate(res, req, CmdMsg::START_TASK);
    };

    auto stopTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: stopTask\n");
        taskOpDelegate(res, req, CmdMsg::STOP_TASK);
    };

    auto deleteTask = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: deleteTask\n");
        taskOpDelegate(res, req, CmdMsg::DELETE_TASK);
    };

    auto enableIORedirect = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: enableIORedirect\n");
        taskOpDelegate(res, req, CmdMsg::ENABLE_REDIRECT);
    };

    auto disableIORedirect = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: disableIORedirect\n");
        taskOpDelegate(res, req, CmdMsg::DISABLE_REDIRECT);
    };

    auto putToStdin = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: putToStdin\n");
        string *bufPtr = new string();
        res->onData([res, bufPtr](string_view data, bool isLast) {
            bufPtr->append(data);
            if (isLast) {
                CmdRes cmdRes{.status = CmdRes::FAILED};
                try {
                    json reqBody = json::parse(*bufPtr);
                    cmdRes = awaitTaskMgrRes(HttpMgr::mCmdSock,
                                             CmdMsg{
                                                 .cmdType = CmdMsg::PUT_TO_STDIN,
                                                 .taskId = reqBody[HTTP_KEY_TASK_ID].get<int32_t>(),
                                                 .stdinContent = std::move(
                                                     reqBody[HTTP_KEY_STDIN_CONTENT].get<string>()),
                                             });
                } catch (const json::exception &e) {
                    fmt::print("__DEBUG putToStdin.onData, json exception: {}\n", e.what());
                }
                if (cmdRes.status == CmdRes::FAILED)
                    writeHttpResp(res, cmdRes, HTTP_400);
                else
                    writeHttpResp(res, cmdRes);
                delete bufPtr;
            }
        });
        res->onAborted([bufPtr]() { delete bufPtr; });
    };

    auto serveIndex = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: serveIndex\n");
        serveFile(string(STATIC_FILE_DIR_PATH) + string("index.html"), res);
    };

    auto serveStatic = [](HttpResponseNoSSL *res, HttpRequest *req) {
        fmt::print("Handler: serveStatic\n");
        serveFile(string(STATIC_FILE_DIR_PATH) +
                      string(req->getUrl().substr(STATIC_FILE_PREFIX.size())),
                  res);
    };

    uWS::App()
        .get("/", serveIndex)
        .get("/index.html", serveIndex)
        .get("/static/*", serveStatic)
        .get("/task", getTask)
        .get("/task/all", getAllTask)
        .get("/task/start", startTask)
        .get("/task/stop", stopTask)
        .get("/task/delete", deleteTask)
        .get("/task/ioredirect/disable", disableIORedirect)
        .get("/task/ioredirect/enable", enableIORedirect)
        .post("/task/create", createTask)
        .post("/task/puttostdin", putToStdin)
        .ws<PerWsData>(
            "/ws", uWS::App::WebSocketBehavior<PerWsData>{
                       .idleTimeout = 120,
                       // BUG uwebsocket 是单线程的
                       .sendPingsAutomatically = false,
                       .upgrade =
                           [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, auto *context) {
                               fmt::print("Handler websocket\n");
                               auto taskIdOpt = checkTaskId(res, req);
                               if (taskIdOpt.has_value()) {
                                   int32_t taskId = taskIdOpt.value();
                                   res->upgrade<PerWsData>(
                                       PerWsData{
                                           .taskId = taskId,
                                       },
                                       req->getHeader("sec-websocket-key"),
                                       req->getHeader("sec-websocket-protocol"),
                                       req->getHeader("sec-websocket-extensions"), context);
                               }
                           },
                       .open =
                           [](uWS::WebSocket<false, true, PerWsData> *ws) {
                               PerWsData *userData = ws->getUserData();
                               userData->ws = ws;
                               fmt::print("Websocket established, taskId: {}\n", userData->taskId);
                               HttpMgr::taskWsHelper.add(userData->taskId, userData);
                           },
                       // BUG uwebsocket 是单线程的，所以客户端不能发送消息，否则可能会崩！
                       .message = [](uWS::WebSocket<false, true, PerWsData> *ws,
                                     string_view message, uWS::OpCode opcode) {},
                       .close =
                           [](uWS::WebSocket<false, true, PerWsData> *ws, int code,
                              std::string_view message) {
                               PerWsData *userData = ws->getUserData();
                               HttpMgr::taskWsHelper.remove(userData->taskId);
                               awaitTaskMgrRes(HttpMgr::mCmdSock,
                                               CmdMsg{.cmdType = CmdMsg::DISABLE_REDIRECT,
                                                      .taskId = userData->taskId});
                           },
                   })
                   
        .listen(PORT,
                [](us_listen_socket_t *listenSock) {
                    if (listenSock)
                        fmt::print("Http server started, listening on port {}\n", PORT);
                })
        .run();
    fmt::print("HttpMgr: http server exited, waiting websocket io data Thread\n");
    close(HttpMgr::mIOSock), close(HttpMgr::mCmdSock);
    wsThread.join();
}

bool TaskWsHelper::sendMsgOnTaskWs(int32_t taskId, const string &msg) {
    lock_guard<mutex> guard(lock);
    bool res = false;
    if (taskId2WsData.find(taskId) != taskId2WsData.end()) {
        try {
            taskId2WsData[taskId]->ws->send(msg, uWS::OpCode::TEXT);
            res = true;
        } catch (const std::exception &e) {
            fmt::print("ws.send msg failed, {}", e.what());
        }
    }
    return res;
}
