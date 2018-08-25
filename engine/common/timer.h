#ifndef ENGINE_COMMON_TIMER_H
#define ENGINE_COMMON_TIMER_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>

namespace engine
{

enum timer_type
{
    TIMER_ONCE      = 0,
    TIMER_CIRCLE    = 1,

}; // enum timer_type

struct timer_task
{
    uint32_t                id;
    uint32_t                interval;
    timer_type              type;
    std::function<void()>   callback;
}; // struct timer_task

#define GRANULARITY 10 // 10 ms
#define WHEEL_BITS1 8
#define WHEEL_BITS2 6
#define WHEEL_SIZE1 (1 << WHEEL_BITS1) // 256
#define WHEEL_SIZE2 (1 << WHEEL_BITS2) // 64
#define WHEEL_MASK1 (WHEEL_SIZE1 - 1)
#define WHEEL_MASK2 (WHEEL_SIZE2 - 1)
#define WHEEL_NUM 5

struct node_link
{
    node_link* prev;
    node_link* next;
    node_link()
    {
        prev = this;
        next = this;
    }
}; // struct node_link

struct timer_node
{
    node_link   link;
    uint64_t    dead_time;
    timer_task  timer;
    timer_node(timer_task t, uint64_t dt)
        : dead_time(dt)
        , timer(t)
    {
    }
}; // struct timer_node

struct wheel
{
    node_link*  spokes;
    uint32_t    size;
    uint32_t    spoke_index;
    explicit wheel(uint32_t n)
        : size(n)
        , spoke_index(0)
    {
        spokes = new node_link[n];
    }

    ~wheel()
    {
        if (spokes) {
            for (uint32_t j = 0; j < size; ++j) {
                node_link* link = (spokes + j)->next;
                while (link != spokes + j) {
                    timer_node* node = (timer_node*) link;
                    link = node->link.next;
                    delete node;
                }
            }
            delete []spokes;
            spokes = nullptr;
        }
    }
}; // struct wheel

static uint64_t get_current_millisec()
{
    auto time_now = std::chrono::system_clock::now();
    auto duration_in_ms = std::chrono::duration_cast
        <std::chrono::milliseconds>(time_now.time_since_epoch());
    return static_cast<uint64_t>(duration_in_ms.count());
}

static std::atomic<uint32_t> auto_timer_increase_id_{0};

static uint32_t get_timer_increase_id()
{
    return ++auto_timer_increase_id_;
}

class timer 
{
public:
    timer()
    {
        checktime_ = get_current_millisec();
        wheels_[0] = new wheel(WHEEL_SIZE1);
        for (int i = 1; i < WHEEL_NUM; ++i) {
            wheels_[i] = new wheel(WHEEL_SIZE2);
        }
    }

    ~timer()
    {
        for (int i = 0; i < WHEEL_NUM; ++i) {
            if (wheels_[i]) {
                delete wheels_[i];
                wheels_[i] = nullptr;
            }
        }
    }

    void detect_timer_list()
    {
        uint64_t now = get_current_millisec();
        uint64_t loopnum = now > checktime_ ? 
            (now - checktime_) / GRANULARITY : 0;

        wheel* wheel = wheels_[0];
        for (uint32_t i = 0; i < loopnum; ++i) {
            node_link* spoke = wheel->spokes + wheel->spoke_index;
            node_link* link = spoke->next;
            while (link != spoke) {
                timer_node* node = (timer_node*)link;
                link->prev->next = link->next;
                link->next->prev = link->prev;
                link = node->link.next;
                add_to_ready_node(node);
            }
            if (++(wheel->spoke_index) >= wheel->size) {
                wheel->spoke_index = 0;
                cascade(1);
            }
            checktime_ += GRANULARITY;
        }
        do_timeout_callback();
    }

    uint32_t add_task(uint32_t interval, timer_type type,
            const std::function<void()>& callback)
    {
        timer_task task;
        task.id         = get_timer_increase_id();
        task.interval   = interval;
        task.type       = type;
        task.callback   = callback;
        timer_map_[task.id] = add_timer(interval, task);
        return task.id;
    }

    void remove_task(uint32_t task_id)
    {
        std::map<uint32_t, timer_node*>::iterator it;
        it = timer_map_.find(task_id);
        if (it != timer_map_.end()) {
            timer_node* node = it->second;
            timer_map_.erase(it);
            node_link* node_link = &(node->link);
            if (node_link->prev) {
                node_link->prev->next = node_link->next; 
            }
            if (node_link->next) {
                node_link->next->prev = node_link->prev;
            }
            node_link->prev = nullptr;
            node_link->next = nullptr;

            delete node;
        }
    }

private:
    timer_node* add_timer(uint32_t milseconds, timer_task timer)
    {
        uint64_t dead_time = get_current_millisec() + milseconds;
        timer_node* node = new timer_node(timer, dead_time);
        add_timer_node(milseconds, node);
        return node;
    }

    uint32_t cascade(uint32_t wheel_index)
    {
        if (wheel_index < 1 || wheel_index >= WHEEL_NUM) {
            return 0;
        }
        wheel* wheel = wheels_[wheel_index];
        uint32_t casnum = 0;
        uint64_t now = get_current_millisec();
        node_link* spoke = wheel->spokes + (wheel->spoke_index++);
        node_link* link = spoke->next;
        spoke->next = spoke->prev = spoke;
        while(link != spoke) {
            timer_node *node = (timer_node*)link;
            link = node->link.next;
            if (node->dead_time <= now) {
                add_to_ready_node(node);
            } else {
                uint64_t milseconds = node->dead_time - now;
                add_timer_node(milseconds, node);
                ++casnum;
            }
        }

        if (wheel->spoke_index >= wheel->size) {
            wheel->spoke_index = 0;
            casnum += cascade(++wheel_index);
        }
        return casnum;
    }

    void add_timer_node(uint64_t milseconds, timer_node* node)
    {
        node_link* spoke = nullptr;
        uint64_t interval = milseconds / GRANULARITY;
        uint32_t threshold1 = WHEEL_SIZE1;
        uint32_t threshold2 = 1 << (WHEEL_BITS1 + WHEEL_BITS2);
        uint32_t threshold3 = 1 << (WHEEL_BITS1 + 2 * WHEEL_BITS2);
        uint32_t threshold4 = 1 << (WHEEL_BITS1 + 3 * WHEEL_BITS2);

        if (interval < threshold1) {
            uint32_t index = (interval + wheels_[0]->spoke_index) 
                    & WHEEL_MASK1;
            spoke = wheels_[0]->spokes + index;
        } else if (interval < threshold2) {
            uint32_t index = ((interval - threshold1 
                    + wheels_[1]->spoke_index * threshold1) 
                    >> WHEEL_BITS1) & WHEEL_MASK2;
            spoke = wheels_[1]->spokes + index;
        } else if (interval < threshold3) {
            uint32_t index = ((interval - threshold2
                    + wheels_[2]->spoke_index * threshold2)
                    >> (WHEEL_BITS1 + WHEEL_BITS2)) & WHEEL_MASK2;
            spoke = wheels_[2]->spokes + index;
        } else if (interval < threshold4) {
            uint32_t index = ((interval - threshold3
                    + wheels_[3]->spoke_index * threshold3)
                    >> (WHEEL_BITS1 + 2 * WHEEL_BITS2)) & WHEEL_MASK2;
            spoke = wheels_[3]->spokes + index;
        } else {
            uint32_t index = ((interval - threshold4
                    + wheels_[4]->spoke_index * threshold4)
                    >> (WHEEL_BITS1 + 3 * WHEEL_BITS2)) & WHEEL_MASK2;
            spoke = wheels_[4]->spokes + index;
        }
        node_link* node_link = &(node->link);
        node_link->prev = spoke->prev;
        spoke->prev->next = node_link;
        node_link->next = spoke;
        spoke->prev = node_link;
    }

    void add_to_ready_node(timer_node* node)
    {
        node_link* node_link = &(node->link);
        node_link->prev = ready_nodes_.prev; 
        ready_nodes_.prev->next = node_link;
        node_link->next = &ready_nodes_;
        ready_nodes_.prev = node_link;
    }

    void do_timeout_callback()
    {
        node_link* link = ready_nodes_.next;
        while (link != &ready_nodes_) {
            timer_node* node = (timer_node*)link;
            if (node->timer.type == TIMER_CIRCLE) {
                timer_map_[node->timer.id] = 
                    add_timer(node->timer.interval, node->timer);
            } else {
                std::map<uint32_t, timer_node*>::iterator it;
                it = timer_map_.find(node->timer.id);
                if (it != timer_map_.end()) {
                    timer_map_.erase(it);
                }
            }
            node->timer.callback();
            link = node->link.next;
            delete node;
        }
        ready_nodes_.next = &ready_nodes_;
        ready_nodes_.prev = &ready_nodes_;
    }

    wheel*                          wheels_[WHEEL_NUM];
    uint64_t                        checktime_;
    node_link                       ready_nodes_;
    std::map<uint32_t, timer_node*> timer_map_;
}; // class timer

} // namespace engine

#endif // ENGINE_COMMON_TIMER_H
