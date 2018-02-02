#include <cstdio>

#include <third_party/g3log/g3log/g3log.hpp>
#include <third_party/g3log/g3log/logworker.hpp>
#include <engine/handler/length_field_base_frame_decoder.h>
#include <engine/net/server.h>

using namespace engine;
using namespace g3;

class handler : public abstract_handler
{
public:
    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        read_data data = any_cast<read_data>(*msg);
        std::string str(data.data + 1, data.len - 1);
        LOGF(INFO, "echo server receive %s", str.c_str());
        data_block block(str.size() + sizeof(uint16_t) + 2);
        std::string test1 = "T", test2 = "E";
        std::copy(test1.c_str(), test1.c_str() + test1.size(), block.data);
        *(reinterpret_cast<uint16_t*>(block.data + 1)) = adapte_endian<uint16_t>(static_cast<uint16_t>(data.len + 3), false);
        std::copy(test2.c_str(), test2.c_str() + test2.size(), block.data + 3);
        std::copy(str.c_str(), str.c_str() + str.size(), block.data + 4);
        write_data w_data(block);
        auto response = new any(w_data);
        ctx->fire_write(std::unique_ptr<any>(response));
        //ctx->fire_write(std::move(msg));
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
        //logworker->addSink(std2::make_unique<FileSink>(argv[0], "./"), &FileSink::fileWrite);
        initializeLogging(logworker.get());

        server s("127.0.0.1", atoi(argv[1]), 10);

        s.set_init_handlers([](std::shared_ptr<session> session){
            session->add_handler("decoder", std::make_shared<length_field_base_frame_decoder>(1024, 1, 2, -3, 3, false))
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
