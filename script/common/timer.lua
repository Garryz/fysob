local modname = "timer"
local M = {}
_G[modname] = M
package.loaded[modname] = M
setmetatable(M, {__index = _G})
_ENV[modname] = M

g_timer = g_timer or {}
g_timer_index = g_timer_index or 0
g_timer_type_once = g_timer_type_once or 0
g_timer_type_circle = g_timer_type_circle or 1

function M.add_timer(interval, timer_type, func, ...)
    g_timer_index = g_timer_index + 1
    g_timer[g_timer_index] = {
        interval = interval,
        timer_type = timer_type,
        func = func,
        arg = { ... }
    }
    return add_timer(interval, timer_type, g_timer_index)
end

return M
