#include "http_mgr.hpp"
#include "buffer_helper.hpp"
#include "cmd_msg.hpp"
#include "config.hpp"
#include "fmt/format.h"
#include "httplib/httplib.h"
#include "util.hpp"

using HttpServer = httplib::Server;
using HttpReq = httplib::Request;
using HttpResp = httplib::Response;

constexpr char HTTP_CONTENT_TYPE_JSON[] = "application/json";

constexpr char HTTP_KEY_TASK_ID[] = "taskId";
constexpr char HTTP_KEY_TITLE[] = "title";
constexpr char HTTP_KEY_SCRIPT_TYPE[] = "scriptType";
constexpr char HTTP_KEY_SCRIPT_CODE[] = "scriptCode";
constexpr char HTTP_KEY_INTERVAL[] = "interval";
constexpr char HTTP_KEY_MAX_TIMES[] = "maxTimes";
constexpr char HTTP_KEY_STDIN_CONTENT[] = "stdinContent";

int HttpConnMgr::cmdSock = 0;
std::mutex HttpConnMgr::sockMutex;

// FIXME 客户端并发的消息与CmdMgr同步的操作不兼容

// send msg to TaskMgr and wait for response
CmdRes awaitTaskMgrRes(int fd, const CmdMsg &msg) {
    std::lock_guard<std::mutex> guard(HttpConnMgr::sockMutex);
    BufferHelper::writeOne(fd, msg.toJson());
    return CmdRes::parse(BufferHelper::readOne(fd));
}

// parse task id from req, return -1 if not found
int32_t checkTaskId(const HttpReq &req) {
    string idParam = req.get_param_value(HTTP_KEY_TASK_ID);
    if (!idParam.empty()) {
        try {
            int32_t taskId = std::stoi(idParam);
            if (taskId != -1)
                return taskId;
        } catch (const std::exception &e) {
            printlnTime("checkTaskId error: {}, ignored", e.what());
        }
    }
    return -1;
}

inline void write400(HttpResp &resp) {
    resp.status = 400;
    resp.set_content(CmdRes{.status = CmdRes::FAILED}.toJsonStr(), HTTP_CONTENT_TYPE_JSON);
}

inline void write200(HttpResp &resp, const string &content) {
    resp.status = 200;
    resp.set_content(content, HTTP_CONTENT_TYPE_JSON);
}

void taskOpImpl(const HttpReq &req, HttpResp &resp, CmdMsg::Type cmdType) {
    int32_t taskId = checkTaskId(req);
    if (taskId == -1) {
        write400(resp);
    } else {
        write200(resp,
                 awaitTaskMgrRes(HttpConnMgr::cmdSock, CmdMsg{.cmdType = cmdType, .taskId = taskId})
                     .toJsonStr());
    }
}

void getTask(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: getTask");
    taskOpImpl(req, resp, CmdMsg::Type::GET_TASK);
}

void getAllTask(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: getAllTask");
    write200(resp,
             awaitTaskMgrRes(HttpConnMgr::cmdSock, CmdMsg{.cmdType = CmdMsg::Type::GET_ALL_TASK})
                 .toJsonStr());
}

void startTask(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: startTask");
    taskOpImpl(req, resp, CmdMsg::Type::START_TASK);
}

void stopTask(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: stopTask");
    taskOpImpl(req, resp, CmdMsg::Type::STOP_TASK);
}

void deleteTask(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: deleteTask");
    taskOpImpl(req, resp, CmdMsg::Type::DELETE_TASK);
}

void enableIORedirect(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: enableIORedirect");
    taskOpImpl(req, resp, CmdMsg::Type::ENABLE_REDIRECT);
}

void disableIORedirect(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: disableIORedirect");
    taskOpImpl(req, resp, CmdMsg::Type::DISABLE_REDIRECT);
}

void createTask(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: createTask");
    CmdRes cmdRes{.status = CmdRes::FAILED};
    try {
        json reqBody = json::parse(req.body);
        cmdRes =
            awaitTaskMgrRes(HttpConnMgr::cmdSock,
                            CmdMsg{
                                .cmdType = CmdMsg::CREATE_TASK,
                                .title = reqBody[HTTP_KEY_TITLE].get<string>(),
                                .scriptCode = reqBody[HTTP_KEY_SCRIPT_CODE].get<string>(),
                                .scriptType = reqBody[HTTP_KEY_SCRIPT_TYPE].get<TaskScriptType>(),
                                .interval = reqBody[HTTP_KEY_INTERVAL].get<int32_t>(),
                                .maxTimes = reqBody[HTTP_KEY_MAX_TIMES].get<int32_t>(),
                            });
    } catch (const json::exception &e) {
        printlnTime("createTask failed, json exception: {}", e.what());
    }
    if (cmdRes.status == CmdRes::FAILED)
        write400(resp);
    else
        write200(resp, cmdRes.toJsonStr());
}

void putToStdin(const HttpReq &req, HttpResp &resp) {
    printlnTime("Http handler: putToStdin");
    CmdRes cmdRes{.status = CmdRes::FAILED};
    try {
        json reqBody = json::parse(req.body);
        cmdRes = awaitTaskMgrRes(
            HttpConnMgr::cmdSock,
            CmdMsg{
                .cmdType = CmdMsg::PUT_TO_STDIN,
                .taskId = reqBody[HTTP_KEY_TASK_ID].get<int32_t>(),
                .stdinContent = std::move(reqBody[HTTP_KEY_STDIN_CONTENT].get<string>()),
            });
    } catch (const json::exception &e) {
        printlnTime("putToStdin failed, json exception: {}", e.what());
    }
    if (cmdRes.status == CmdRes::FAILED)
        write400(resp);
    else
        write200(resp, cmdRes.toJsonStr());
}

void HttpConnMgr::start(int cmdSock) {
    HttpConnMgr::cmdSock = cmdSock;
    HttpServer server;
    if (server.set_mount_point(STATIC_FILE_PREFIX, STATIC_FILE_DIR_PATH) == false) {
        printlnTime("HttpMgr: set mount point failed!");
        return;
    }
    server
        .Get("/", [](const auto &req, HttpResp &resp) { resp.set_redirect("/static/index.html"); })
        .Get("/task", getTask)
        .Get("/task/all", getAllTask)
        .Get("/task/start", startTask)
        .Get("/task/stop", stopTask)
        .Get("/task/delete", deleteTask)
        .Get("/task/ioredirect/disable", disableIORedirect)
        .Get("/task/ioredirect/enable", enableIORedirect)
        .Post("/task/create", createTask)
        .Post("/task/puttostdin", putToStdin);
    println("Http server started on port: {}", PORT);
    server.listen("0.0.0.0", PORT, SO_REUSEADDR);
    println("HttpServer failed to start");
}
