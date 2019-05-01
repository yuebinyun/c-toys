#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>
#include "WsClient.h"
//#include <spdlog/sinks/daily_file_sink.h>
//#include <spdlog/sinks/stdout_sinks.h>

class App : public WsClientCallback {

private:
    WsClient *wsClient;

public:
    void onConnSuccess(std::string *sessionId) override {
        SPDLOG_DEBUG("ws 连接成功");
        std::string ban = "Ban";
        wsClient->sendMessage(&ban);
    }

    void onConnClosed() override {
        SPDLOG_DEBUG("ws 连接关闭");
    }

    void onConnError() override {
        SPDLOG_ERROR("ws 连接异常");
    }

    void onReceiveMessage(std::string *message) override {
        SPDLOG_INFO("ws 收到消息 {}", message->c_str());
    };

    App() {
        wsClient = nullptr;
    }

    void start() {
        std::string *address = new std::string("localhost");
        std::string *path = new std::string("/");
        wsClient = new WsClient(address, 8765, path, this);
        delete address;
        delete path;

        wsClient->start();

        sleep(10);

        wsClient->stop();
        delete wsClient;
        wsClient = nullptr;
    }
};

int main(int args, char **argv) {

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e %z] [%t] [%@] %v");
    spdlog::set_level(spdlog::level::trace); // Set specific logger's log level
    SPDLOG_DEBUG("Welcome to spdlog version {}.{}.{} !",
                 SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

//    auto console = spdlog::stdout_logger_mt("console");
//    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/daily.txt", 2, 30);
//    spdlog::set_default_logger(daily_logger);

    (new App())->start();
    return 0;
}

