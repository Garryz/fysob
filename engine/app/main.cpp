#include <cstdio>
#include <string>

#include <third_party/g3log/g3log/g3log.hpp>
#include <third_party/g3log/g3log/logworker.hpp>
#include <engine/common/common.h>
#include <engine/handler/length_field_base_frame_decoder.h>
#include <engine/net/net_manager.h>
#include <engine/net/server.h>

using namespace engine;
using namespace g3;

class handler : public abstract_handler
{
public:
    handler(const handler&) = delete;
    handler& operator=(const handler&) = delete;
    explicit handler()
    {
    }

    virtual void connect(context* ctx)
    {
        net_manager::on_connect(ctx);    
    }

    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        read_data data = any_cast<read_data>(*msg);
        net_manager::on_message(ctx->session_id(), data.data, data.len);
    }
};

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: app config.lua\n");
        return 1;
    }

    char* filename = argv[1];
    
    std::unique_ptr<LogWorker> logworker{ LogWorker::createLogWorker() };
    //logworker->addSink(std2::make_unique<FileSink>(argv[0], "./"), &FileSink::fileWrite);
    initializeLogging(logworker.get());

    net_manager::init();
    lua_State* L = net_manager::get_lua_state();

    if (luaL_loadfile(L, filename) || lua_pcall(L,0,0,0)) {
        luaL_error(L, "loadfile error! %s \n", lua_tostring(L, -1));
        return 1;
    }

    lua_getglobal(L,"g_config");
    if (!lua_istable(L, -1)) {
        luaL_error(L, "loadfile error! %s \n", lua_tostring(L, -1));
        return 1;
    }
    lua_getfield(L, -1, "app_type");

    int app_type = static_cast<int>(lua_tonumber(L, -1));
    if (app_type == static_cast<int>(engine::AppType::GAME)) {
        lua_pop(L, 1);
        lua_getfield(L, -1, "ip");
        if (!lua_isstring(L, -1)) {
            luaL_error(L, "loadfile error! %s \n", lua_tostring(L, -1));
            return 1;
        }
        std::string ip = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "port");
        int port = (int)lua_tonumber(L, -1);
        lua_pop(L, 1);

        printf("app type = %d, ip = %s, port = %d\n", app_type, ip.c_str(), port);

        const char* main_lua = "./script/game/main.lua";
        if (luaL_loadfile(L, main_lua) || lua_pcall(L,0,0,0)) {
            luaL_error(L, "loadfile main.lua error! %s \n", lua_tostring(L, -1));
            return 1;
        }

        server s(ip.c_str(), port, 10);

        s.set_init_handlers([](std::shared_ptr<session> session){
            session->add_handler("decoder", std::make_shared<length_field_base_frame_decoder>(1024, 0, 2, 0, 2))
                ->add_handler("server_handler", std::make_shared<handler>());
        });

        s.run();

        getchar();

        s.stop();
    }

    net_manager::close();

    return 0;
}
