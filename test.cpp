#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <atomic>
#include <string>
#include <json/json.h>

#include <workflow/HttpMessage.h>
#include <workflow/HttpUtil.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/WFFacilities.h>

#pragma comment(lib, "Ws2_32")
#pragma comment(lib, "Mswsock")
#pragma comment(lib, "openssl")
#pragma comment(lib, "libssl")
#pragma comment(lib, "libcrypto")
#pragma comment(lib, "workflow")
#pragma comment(lib, "jsoncpp")

#define REDIRECT_MAX    5
#define RETRY_MAX       0

std::atomic_uint64_t g_count{ 0 };

void test_http_task_cb(WFHttpTask* task) {
    protocol::HttpRequest* req = task->get_req();
    protocol::HttpResponse* resp = task->get_resp();
    int state = task->get_state();
    int error = task->get_error();
    switch(state) {
    case WFT_STATE_SYS_ERROR:
        fprintf(stderr, "system error: %s\n", strerror(error));
        break;
    case WFT_STATE_DNS_ERROR:
        fprintf(stderr, "DNS error: %s\n", gai_strerror(error));
        break;
    case WFT_STATE_SSL_ERROR:
        fprintf(stderr, "SSL error: %d\n", error);
        break;
    case WFT_STATE_TASK_ERROR:
        fprintf(stderr, "Task error: %d\n", error);
        break;
    case WFT_STATE_SUCCESS:
        break;
    }
    if(state != WFT_STATE_SUCCESS) {
        fprintf(stderr, "fail to deal http task, state: %d, error: %d\n", state, error);
    } else {
        const void* body;
        size_t body_len;
        resp->get_parsed_body(&body, &body_len);
        fwrite(body, 1, body_len, stdout);
        fflush(stdout);
        fprintf(stderr, "\nSuccess. Press Ctrl-C to exit.\n");
    }
    //if(g_count++ < 5) {
    //    Json::Value root;
    //    root["signatureStr"] = "butel";
    //    root["signatureVersion"] = "butel";
    //    root["meetingID"] = "butel";
    //    root["funcName"] = "butel";
    //    const char* url = "http://10.33.36.72:8081/testUrl";
    //    WFHttpTask* http_task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, RETRY_MAX,
    //                                                            test_http_task_cb);
    //    http_task->set_send_timeout(2000);
    //    http_task->set_receive_timeout(2000);
    //    protocol::HttpRequest* req = http_task->get_req();
    //    req->add_header_pair("Accept", "*/*");
    //    // req->add_header_pair("Connection", "close");
    //    req->set_method("POST");
    //    req->append_output_body(root.toStyledString());
    //    // http_task->start();
    //    *series_of(task) << http_task;
    //}
}

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo) {
    wait_group.done();
}

int main(int argc, char* argv[]) {
    signal(SIGINT, sig_handler);
    struct WFGlobalSettings settings = GLOBAL_SETTINGS_DEFAULT;
    settings.endpoint_params.max_connections = 20000;
    settings.endpoint_params.response_timeout = -1;
    WORKFLOW_library_init(&settings);
    for(int i = 0; i < 3; ++i) {
        Json::Value root;
        root["signatureStr"] = "butel";
        root["signatureVersion"] = "butel";
        root["meetingID"] = "butel";
        root["funcName"] = "butel";
        const char* url = "http://10.33.36.72:8081/MeetingCtrl";
        WFHttpTask* http_task = WFTaskFactory::create_http_task(url, REDIRECT_MAX, RETRY_MAX,
                                                                test_http_task_cb);
        http_task->set_send_timeout(2000);
        http_task->set_receive_timeout(2000);
        protocol::HttpRequest* req = http_task->get_req();
        req->add_header_pair("Accept", "*/*");
        // req->add_header_pair("Connection", "close");
        req->set_method("POST");
        req->append_output_body(root.toStyledString());
        http_task->start();
    }
    wait_group.wait();
    printf("exit suc\n");
    return 0;
}
