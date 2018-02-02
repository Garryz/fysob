#include <cstdio>

#include <third_party/g3log/g3log/g3log.hpp>
#include <third_party/g3log/g3log/logworker.hpp>
#include <engine/handler/delimiter_based_frame_decoder.h>
#include <engine/net/server.h>

using namespace engine;
using namespace g3;

class handler : public abstract_handler
{
public:
    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        read_data data = any_cast<read_data>(*msg);
        std::string str(data.data, data.len);
        LOGF(INFO, "echo server receive %s", str.c_str());
        ctx->fire_write(std::move(msg));
    }
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            printf("Usage: echo_server <port> \n");
            return 1;
        }

        std::unique_ptr<LogWorker> logworker{ LogWorker::createLogWorker() };
        logworker->addSink(std2::make_unique<FileSink>(argv[0], "./"), &FileSink::fileWrite);
        initializeLogging(logworker.get());

        server s("127.0.0.1", atoi(argv[1]), 10);

        s.set_init_handlers([](std::shared_ptr<session> session){
            session->add_handler("decoder", std::make_shared<delimiter_based_frame_decoder>(1024, "\n", false))
                ->add_handler("echo", std::make_shared<handler>());
        });

        s.run();

        getchar();

        s.stop();
    } catch (std::exception& e) {
        printf("Exception：%s \n", e.what());
    }
    return 0; 
}
