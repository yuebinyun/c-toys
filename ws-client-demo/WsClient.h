#pragma once

#include "libwebsockets.h"
#include <string>
#include <map>
#include "queue.h"

#include <list>

class WsClientCallback {
public:
    virtual void onConnSuccess(std::string *sessionId) = 0;

    virtual void onConnClosed() = 0;

    virtual void onConnError() = 0;

    virtual void onReceiveMessage(std::string *message) = 0;

    virtual ~WsClientCallback() = default;
};

class WsClient {

public:
    WsClient(std::string *address, int port, std::string *path, WsClientCallback *callback);

    ~WsClient();

    void start();

    void stop();

    void sendMessage(std::string *message);

private:

    std::string address;
    int port;
    std::string path;

    // ws 网络收发线程
    std::thread *pt;

    // 断网重连的时候会用到
    struct lws *wsClient;

    // 线程退出标志 stop && noMsg
    volatile bool stop_;

    // noMsg 用于尽可能保证离开房间的消息在线程退出前已经发出
    volatile bool noMsg;

    struct lws_protocols protocols_[2];

    // 用于存贮网络接受到的消息
    std::string message;

    // 待发消息队列
    Queue<std::string *> messageQueue;

    // 回调，请不要在回调内处理耗时的操作
    WsClientCallback *lwsCallback;

private:

    static int on_websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
                                     void *user_data, void *in, size_t len);

    static void netThread(WsClient *pSelf);

    void netTask();
};

