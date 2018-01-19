#include <iostream>

#include <third_party/g3log/g3log/g3log.hpp>
#include <third_party/g3log/g3log/logworker.hpp>
#include <engine/handler/abstract_handler.h>
#include <engine/net/server.h>
#include <engine/net/client.h>

using namespace engine;

class handler : public abstract_handler
{
public:
    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        auto buffer = any_cast<std::shared_ptr<asio_buffer>>(*msg);
        assert(buffer);
        assert(buffer->readable_bytes() >= sizeof(char));
        char result = buffer->read<char>();
        LOGF(INFO, "result = %c, readable_bytes = %d", result, buffer->readable_bytes());
        auto response = new any(result);
        ctx->write(std::unique_ptr<any>(response)); 
        //ctx->close();
    }   
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: server <port>\n";
            return 1;
        }

        using namespace std;
        using namespace engine;
        using namespace g3;

        std::unique_ptr<LogWorker> logworker{ LogWorker::createLogWorker() };
        logworker->addSink(std2::make_unique<FileSink>(argv[0], "./"), &FileSink::fileWrite);
        initializeLogging(logworker.get());
    
        client c("127.0.0.1", atoi(argv[1]));

        server s("127.0.0.1", atoi(argv[1]), 10);

        s.set_read_high_water_mask(3);

        s.set_init_handlers([](std::shared_ptr<session> session){
            session->add_handler("test", std::make_shared<handler>());        
        });

        s.run();

        getchar();

        s.stop();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
