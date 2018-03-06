#include <third_party/g3log/g3log/g3log.hpp>
#include <third_party/g3log/g3log/logworker.hpp>
#include <engine/handler/length_field_base_frame_decoder.h>
#include <engine/net/client.h>

using namespace engine;
using namespace g3;

int main(int argc, char* argv[])
{
    try {
        if (argc != 3) {
            printf("Usage: client <host> <port> \n");
            return 1;
        }

        std::unique_ptr<LogWorker> logworker{ LogWorker::createLogWorker() };
        //logworker->addSink(std2::make_unique<FileSink>(argv[0], "./"), &FileSink::fileWrite);
        initializeLogging(logworker.get());

        client c(argv[1], atoi(argv[2]));

        //c.set_init_handlers([](std::shared_ptr<session> session){
        //    session->add_handler("decoder", std::make_shared<length_field_base_frame_decoder>(1024, 0, 2, 0, 2));        
        //}); 

        c.run();

        const char* str = "test echo";
        c.get_session()->write<uint16_t>(strlen(str));
        c.get_session()->write(str);

        getchar();

        c.stop();
    } catch (std::exception& e) {
        printf("exception: %s \n", e.what());
    }

    return 0;
}
