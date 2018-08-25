#ifndef ENGINE_NET_NET_MANAGER_H
#define ENGINE_NET_NET_MANAGER_H

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <map>
#include <mutex>
#include <third_party/asio.hpp>
#include <third_party/asio/steady_timer.hpp>
#include <third_party/g3log/g3log/g3log.hpp>
#include <engine/common/timer.h>
#include <engine/handler/context.h>
#include <engine/net/io_service_pool.h>

namespace engine
{

class net_manager
{
public:
    static void init()
    {   
        lua_state_ = luaL_newstate();
        luaL_openlibs(lua_state_);

        lua_register(lua_state_, "write_message", net_manager::write_message);

        lua_register(lua_state_, "close_connection", net_manager::close_connection);

        lua_register(lua_state_, "add_timer", net_manager::add_timer);

        lua_register(lua_state_, "remove_timer", net_manager::remove_timer);

        check_timer();

        timer_service_pool_.run();
    }

    static lua_State* get_lua_state()
    {
        return lua_state_;
    }

    static void close()
    {
        lua_close(lua_state_);

        timer_service_pool_.stop();
    }

    static void check_timer()
    {
        asio_timer_.expires_from_now(std::chrono::seconds(30));
        asio_timer_.async_wait([=](std::error_code ec){handle_check_timer(ec);});
    }

    static void handle_check_timer(std::error_code& ec)
    {
        if (!ec) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                timer_.detect_timer_list();
            }
        } else {
            LOGF(FATAL, "check timer error = %s", ec.message().c_str());
        }

        check_timer();
    }

    static void on_connect(context* ctx)
    {
        std::lock_guard<std::mutex> lock(mutex_); 
        context_map_[ctx->session_id()] = ctx;
        lua_getglobal(lua_state_, "on_connect");
        lua_pushinteger(lua_state_, ctx->session_id());

        if (lua_pcall(lua_state_, 1, 0, 0) != 0) {
            luaL_error(lua_state_, "on_connect error! %s \n", lua_tostring(lua_state_, -1)); 
        }
    }

    static void on_message(uint32_t session_id, const char* msg, std::size_t msg_len)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lua_getglobal(lua_state_, "on_message");
        lua_pushinteger(lua_state_, session_id);
        lua_pushlstring(lua_state_, msg, msg_len);

        if (lua_pcall(lua_state_, 2, 0, 0) != 0) {
            luaL_error(lua_state_, "on_message error! %s \n", lua_tostring(lua_state_, -1));
        }
    }

    static void on_passive_clean(uint32_t session_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::map<uint32_t, context*>::iterator it;
        it = context_map_.find(session_id);
        if (it != context_map_.end()) {
            context_map_.erase(it);
        }

        lua_getglobal(lua_state_, "on_passive_clean");
        lua_pushinteger(lua_state_, session_id);

        if (lua_pcall(lua_state_, 1, 0, 0) != 0) {
            luaL_error(lua_state_, "on_passive_clean error! %s \n", lua_tostring(lua_state_, -1));
        }
    }

    static int write_message(lua_State* L)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t session_id = luaL_checkinteger(L, 1);
        std::map<uint32_t, context*>::iterator it;
        it = context_map_.find(session_id);
        if (it != context_map_.end()) {
            read_data msg;
            msg.data = luaL_checklstring(L, 2, &msg.len);
            auto response = new any(msg);
            it->second->fire_write(std::unique_ptr<any>(response));
        }
        return 0;
    }

    static int close_connection(lua_State* L)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t session_id = luaL_checkinteger(L, 1);
        context* result = nullptr;
        std::map<uint32_t, context*>::iterator it;
        if (it != context_map_.end()) {
            result = it->second;
            context_map_.erase(it);
            result->fire_close();
        }
        return 0;
    }

    static int add_timer(lua_State* L)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t interval   = luaL_checkinteger(L, 1);
        uint32_t type       = luaL_checkinteger(L, 2);
        uint32_t index      = luaL_checkinteger(L, 3);

        uint32_t id = timer_.add_task(interval, static_cast<timer_type>(type), 
                [type, index](){
                    
                });
    }

    static int remove_timer(lua_State* L)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t index      = luaL_checkinteger(L, 1);

    }
private:
    static lua_State*                   lua_state_;
    static std::map<uint32_t, context*> context_map_;
    static timer                        timer_;
    static std::mutex                   mutex_;
    static io_service_pool              timer_service_pool_;
    static asio::steady_timer           asio_timer_;
}; // class net_manager

lua_State* net_manager::lua_state_;
std::map<uint32_t, context*> net_manager::context_map_;
timer net_manager::timer_;
std::mutex net_manager::mutex_;
io_service_pool net_manager::timer_service_pool_(1, "timer_pool");
asio::steady_timer net_manager::asio_timer_(timer_service_pool_.get_io_service());

} // namespace engine

#endif // ENGINE_NET_NET_MANAGER_H
