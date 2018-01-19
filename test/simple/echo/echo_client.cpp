#include <cstdio>
#include <random>

#include <third_party/g3log/g3log/g3log.hpp>
#include <third_party/g3log/g3log/logworker.hpp>
#include <engine/handler/delimiter_based_frame_decoder.h>
#include <engine/net/client.h>

using namespace engine;
using namespace g3;

class handler : public abstract_handler
{
public:
    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        read_data data = any_cast<read_data>(*msg);
        std::string str(data.data, data.len);
        LOGF(INFO, "echo client receive %s", str.c_str());
        std::string joker; 
        joker.append(std::to_string(rd_())).append(" fool jokers \n");
        LOGF(INFO, "current joker string = %s", joker.c_str());
        auto response = new any(joker);
        ctx->fire_write(std::unique_ptr<any>(response));
    }
private:
    std::random_device rd_;
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 3) {
            printf("Usage: echo_server <host> <port> \n");
            return 1;
        }

        std::unique_ptr<LogWorker> logworker{ LogWorker::createLogWorker() };
        logworker->addSink(std2::make_unique<FileSink>(argv[0], "./"), &FileSink::fileWrite);
        initializeLogging(logworker.get());

        client c(argv[1], atoi(argv[2]));

        c.set_init_handlers([](std::shared_ptr<session> session){
            session->add_handler("decoder", std::make_shared<delimiter_based_frame_decoder>(1024, "\n", false))
                ->add_handler("echo", std::make_shared<handler>());        
        });

        c.run();

        c.get_session()->write("test echo \n");

        getchar();

        c.stop();
    } catch (std::exception& e) {
        printf("exception: %s \n", e.what());
    }

    return 0;
}
