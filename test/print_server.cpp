#include <iostream>
#include <engine/handle/abstract_handler.h>
#include <engine/net/server.h>

using namespace engine;

class handler : public abstract_handler
{
public:
    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        printf("session id = %i \n", ctx->session_id());
        any user_data = ctx->get_user_data();
        printf("user data = %s \n", any_cast<const char*>(user_data));
        auto buffer = any_cast<std::shared_ptr<asio_buffer>>(*msg);
        std::unique_ptr<data_block> free_data = buffer->read(buffer->readable_bytes());
        printf("receive: %i bytes. message: %s \n", free_data->len, free_data->data);
        read_data_ptr read_data;
        read_data.data = free_data->data;
        read_data.len = free_data->len;
        auto result = new any(read_data);
        ctx->fire_write(std::unique_ptr<any>(result));
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

        server s("127.0.0.1", atoi(argv[1]), 10);

        s.set_init_handlers([](std::shared_ptr<session> session){
                    session->add_handler(std::make_shared<handler>())
                        ->set_user_data("test");
                });

        s.run();

        getchar();

        s.stop();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
