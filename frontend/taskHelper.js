var TaskHelper = (function () {

    var CmdType2Url = [];

    const GET_ALL_TASK = 0, GET_TASK = 1, START_TASK = 2,
        STOP_TASK = 3, DELETE_TASK = 4, DISABLE_REDIRECT = 5,
        ENABLE_REDIRECT = 6, CREATE_TASK = 7, PUT_TO_STDIN = 8;

    CmdType2Url[GET_ALL_TASK] = "/task/all";
    CmdType2Url[GET_TASK] = "/task";
    CmdType2Url[START_TASK] = "/task/start";
    CmdType2Url[STOP_TASK] = "/task/stop";
    CmdType2Url[DELETE_TASK] = "/task/delete";
    CmdType2Url[DISABLE_REDIRECT] = "/task/ioredirect/disable";
    CmdType2Url[ENABLE_REDIRECT] = "/task/ioredirect/enable";
    CmdType2Url[CREATE_TASK] = "/task/create";
    CmdType2Url[PUT_TO_STDIN] = "/task/puttostdin";


    const CmdResType2Str = [
        "OK", "任务未运行", "任务未找到",
        "任务正在运行", "失败", "无效的任务类型"
    ];

    function handleFetch(fetchRes) {
        return fetchRes.then((resp) => {
            if (resp.ok) return resp.json();
            else throw resp.status + " " + resp.statusText;
        });
    }


    function taskOpImpl(cmdType, taskId) {
        return handleFetch(fetch(CmdType2Url[cmdType] + "?taskId=" + taskId));
    };

    function getAllTask() { return handleFetch(fetch(CmdType2Url[GET_ALL_TASK])); };
    function getTask(taskId) { return taskOpImpl(GET_TASK, taskId); }
    function startTask(taskId) { return taskOpImpl(START_TASK, taskId); }
    function stopTask(taskId) { return taskOpImpl(STOP_TASK, taskId); }
    function deleteTask(taskId) { return taskOpImpl(DELETE_TASK, taskId); }
    function disableRedirect(taskId) { return taskOpImpl(DISABLE_REDIRECT, taskId); }
    function enableRedirect(taskId) { return taskOpImpl(ENABLE_REDIRECT, taskId); }
    function createTask(title, scriptCode, scriptType, interval, maxTimes) {
        return handleFetch(fetch(CmdType2Url[CREATE_TASK], {
            method: 'POST',
            body: JSON.stringify({
                title, scriptCode, scriptType,
                interval, maxTimes,
            }),
            headers: {
                'Content-Type': 'application/json;charset=utf-8;'
            }
        }));
    };
    function putToStdin(obj) {
        return handleFetch(fetch(CmdType2Url[PUT_TO_STDIN], {
            method: 'POST',
            body: JSON.stringify({
                taskId: obj.taskId,
                stdinContent: obj.stdinContent
            }),
            headers: {
                'Content-Type': 'application/json;charset=utf-8;'
            }
        }));
    };
    return {
        getAllTask, getTask, startTask, stopTask, deleteTask,
        enableRedirect, disableRedirect, createTask, putToStdin,
        CmdResType2Str,
    };
})();