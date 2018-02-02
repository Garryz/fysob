#include <cstdio>
#include <random>

#include <third_party/g3log/g3log/g3log.hpp>
#include <third_party/g3log/g3log/logworker.hpp>
#include <engine/handler/length_field_base_frame_decoder.h>
#include <engine/net/client.h>
#include <engine/net/endian.h>

using namespace engine;
using namespace g3;

class handler : public abstract_handler
{
public:
    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        read_data data = any_cast<read_data>(*msg);
        std::string str(data.data + 1, data.len - 1);
        LOGF(INFO, "echo client receive %s", str.c_str());
        std::string joker; 
        joker.append(std::to_string(rd_())).append(" fool jokers");
        uint16_t len = adapte_endian<uint16_t>(static_cast<uint16_t>(joker.size() + 4), false);
        data_block block(joker.size() + sizeof(uint16_t) + 2);
        std::string test1 = "T", test2 = "E";
        std::copy(test1.c_str(), test1.c_str() + test1.size(), block.data);
        *(reinterpret_cast<uint16_t*>(block.data + 1)) = len;
        std::copy(test2.c_str(), test2.c_str() + test2.size(), block.data + 3);
        std::copy(joker.c_str(), joker.c_str() + joker.size(), block.data + 4);
        write_data w_data(block);
        auto response = new any(w_data);
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
        //logworker->addSink(std2::make_unique<FileSink>(argv[0], "./"), &FileSink::fileWrite);
        initializeLogging(logworker.get());

        client c(argv[1], atoi(argv[2]));

        c.set_init_handlers([](std::shared_ptr<session> session){
            session->add_handler("decoder", std::make_shared<length_field_base_frame_decoder>(1024, 1, 2, -3, 3, false))
                ->add_handler("echo", std::make_shared<handler>());        
        });

        c.run();
   
        const char* str = "test echo";
        c.get_session()->write("T");
        c.get_session()->write_endian<uint16_t>(strlen(str) + 4, false);
        c.get_session()->write("E");
        c.get_session()->write(str);

        getchar();

        c.stop();
    } catch (std::exception& e) {
        printf("exception: %s \n", e.what());
    }

    return 0;
}
