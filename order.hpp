#ifndef ORDER_HPP
#define ORDER_HPP

#include <cstdint>
#include <folly/IntrusiveList.h>
#include <stdexcept>
#include <vector>
#include "tick.hpp"
using OrderNo_t = int64_t;

struct Order {
    int64_t price;
    int64_t volume;
    int64_t tp;
    char b_s;
    int64_t org;
    int64_t orderNo;
    Order(int64_t p, int64_t vol, int64_t tp, char b_s, int64_t org)
        : price(p), volume(vol), tp(tp), b_s(b_s), org(org){};
    Order() = default;
    void fill_order_from_tick(TickInfo *tick);
};


struct IntrusiveOrder : public Order {
    folly::IntrusiveListHook hook;
    using Order::fill_order_from_tick;
    using Order::Order;
    bool operator==(const IntrusiveOrder &other) const {
        return orderNo == other.orderNo;// 比较两个订单的id是否相同
    }
};

using IntrusiveOrderList = folly::IntrusiveList<IntrusiveOrder, &IntrusiveOrder::hook>;

class OrderPool {
public:
    std::vector<IntrusiveOrder> orders_pool;
    IntrusiveOrderList free_list;// 空闲订单列表
    OrderPool(size_t size) {
        orders_pool.resize(size);
        for (auto &order: orders_pool) {
            free_list.push_back(order);
        }
    }

public:
    IntrusiveOrder &get_order() {
        if (!free_list.empty()) {
            IntrusiveOrder &order = free_list.front();
            free_list.pop_front();
            return order;
        } else {
            throw std::runtime_error("memory exhausted");
        }
    }
    void recycle_order(IntrusiveOrder &order) {
        free_list.push_back(order);
    }
};


#endif