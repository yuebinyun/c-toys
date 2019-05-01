#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <utility>
#include "WsClient.h"
#include "iostream"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

WsClient::WsClient(std::string *address, int port, std::string *path, WsClientCallback *callback) {

    this->address = std::string(*address);
    this->port = port;
    this->path = std::string(*path);

    SPDLOG_DEBUG("[入参] url = {}:{}{}", this->address, this->port, this->path);

    this->pt = nullptr;
    this->stop_ = false;
    this->noMsg = true;
    this->lwsCallback = callback;

    protocols_[0] = {
            "kktv-protocol",
            on_websocket_callback,
            0,
            20,
            0,
            this,
            0
    };

    protocols_[1] = {
            nullptr,
            nullptr,
            0,
            0
    };
}

// join 操作，确保线程退出
WsClient::~WsClient() {
    // 尝试停止
    stop();
}

int WsClient::on_websocket_callback(struct lws *wsi,
                                    enum lws_callback_reasons reason,
                                    void *user_data,
                                    void *in,
                                    size_t len) {

    auto *self = (WsClient *) user_data;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            SPDLOG_DEBUG("[ws 连接成功]");
            if (self->lwsCallback) {
                self->lwsCallback->onConnSuccess(nullptr);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
            SPDLOG_DEBUG("[ws 连接错误]");
            if (self->lwsCallback)
                self->lwsCallback->onConnError();
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            SPDLOG_DEBUG("[ws 连接关闭]");
            self->wsClient = nullptr;
            if (self->lwsCallback)
                self->lwsCallback->onConnClosed();
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            if (len > 0) {
                self->message.append((const char *) in, len);
                if (lws_is_final_fragment(wsi)) {
                    self->lwsCallback->onReceiveMessage(&self->message);
                    self->message.clear();
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            if (!self->messageQueue.empty()) {
                // 发送消息队列里的消息
                std::string *message = self->messageQueue.pop();
                unsigned char buf[LWS_PRE + message->length() + LWS_SEND_BUFFER_POST_PADDING];
                unsigned char *p = &buf[LWS_PRE];
                auto n = static_cast<size_t>(sprintf((char *) p, "%s", message->c_str()));
                int ret = lws_write(wsi, p, n, LWS_WRITE_TEXT);
                SPDLOG_DEBUG("[ws 发送消息] {}", message->c_str());
                delete message;
                self->noMsg = self->messageQueue.empty();
            } else {
                SPDLOG_DEBUG("发送保活消息");
                uint8_t ping[LWS_PRE + 125];
                lws_write(wsi, ping + LWS_PRE, 0, LWS_WRITE_PING);
            }
            break;
        }
        case LWS_CALLBACK_WSI_DESTROY: {
            SPDLOG_DEBUG("[ws 连接销毁]");
            self->wsClient = nullptr;
            break;
        }
        default: {
//            RTC_LOG(LERROR) << "on_websocket_callback" << reason;
            break;
        }
    }
    return 0;
}

void WsClient::sendMessage(std::string *message) {
    auto *msg = new std::string(*message);
    messageQueue.push(msg);
    noMsg = false;
    if (wsClient) {
        lws_callback_on_writable(wsClient);
    }
}

void WsClient::netThread(WsClient *pSelf) {
    pSelf->netTask();
}

/**
 * 网络读写进程
 */
void WsClient::netTask() {

    SPDLOG_DEBUG("[ws 线程开始] {}", pthread_self());

    struct lws_context_creation_info ctxInfo{};
    memset(&ctxInfo, 0, sizeof(ctxInfo));
    ctxInfo.port = CONTEXT_PORT_NO_LISTEN;
    ctxInfo.protocols = protocols_;
    ctxInfo.extensions = nullptr;
    ctxInfo.gid = -1;
    ctxInfo.uid = -1;
    ctxInfo.ws_ping_pong_interval = 20;
    ctxInfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    ctxInfo.timeout_secs = 20;
    struct lws_context *wsContext = lws_create_context(&ctxInfo);

    if (wsContext == nullptr) {
        SPDLOG_DEBUG("[ERROR] lws_create_context() 失败 ");
        SPDLOG_DEBUG("[ws 线程退出]");
        return;
    }

    struct lws_client_connect_info clientInfo{};
    memset(&clientInfo, 0, sizeof(clientInfo));
    clientInfo.context = wsContext;
    clientInfo.ssl_connection = 0;
    clientInfo.host = lws_canonical_hostname(wsContext);
    clientInfo.address = this->address.c_str();
    clientInfo.port = this->port;
    clientInfo.path = this->path.c_str();
    clientInfo.origin = "origin";
    clientInfo.ietf_version_or_minus_one = -1;
    clientInfo.protocol = protocols_[0].name;
    clientInfo.userdata = this;
    wsClient = lws_client_connect_via_info(&clientInfo);

    int countdown = 5;
    while (!stop_ || !noMsg) {
        if (!wsClient) {
            sleep(1);
            --countdown;
            if (stop_) break;
            if (countdown == 0) {
                SPDLOG_DEBUG("[ws 重连] url = {}:{}{}", this->address, this->port, this->path);
                wsClient = lws_client_connect_via_info(&clientInfo);
                countdown = 5;
            } else {
                continue;
            }
        }
        lws_service(wsContext, 50);
    }

    lws_context_destroy(wsContext);
    wsClient = nullptr;

    SPDLOG_DEBUG("[ws 线程退出]");
}

void WsClient::start() {
    if (pt != nullptr) {
        SPDLOG_DEBUG("[ERROR] ws 线程已经运行");
    } else {
        pt = new std::thread(netThread, this);
    }
}

void WsClient::stop() {
    stop_ = true;
    if (pt && pt->joinable()) {
        pt->join();
    }
    if (pt) {
        delete pt;
        pt = nullptr;
    }
}