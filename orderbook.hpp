#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP
#include <absl/container/btree_map.h>
#include <absl/container/flat_hash_map.h>
#include <absl/log/absl_log.h>
#include <absl/log/log.h>
#include <cstdint>
#include <folly/IntrusiveList.h>
#include <folly/ProducerConsumerQueue.h>//SPSC queue
#include <memory>
#include <optional>
#include <stdexcept>
#include "credit.hpp"
#include <vector>
#include <fmt/format.h>
#include "fmt/core.h"
#include "order.hpp"


using SparseMatrix = absl::flat_hash_map<std::pair<int, int>, int64_t>;
using SparseMatrixElement = absl::flat_hash_map<std::pair<int, int>, int64_t>::iterator;
class OrgContribution {
public:
    absl::flat_hash_map<std::pair<int, int>, int64_t> contribution_volumes_;

public:
    std::optional<SparseMatrixElement> get_contribution_item(int from, int to);
};


class PriceLeader {
private:
    void remove(IntrusiveOrder &order);
public:
    int64_t price;
    int64_t vol;
    IntrusiveOrderList order_list;
    int64_t executable_vol;
    std::unique_ptr<std::vector<int64_t>> org_executable_vols;
    std::unique_ptr<std::vector<int64_t>> org_aggregated_vol;
    std::unique_ptr<OrgContribution> org_contribution;
    std::shared_ptr<OrderPool> order_pool;
    explicit PriceLeader(int orgs);
    PriceLeader() = delete;
    void clear();
    std::optional<IntrusiveOrder *> find_order(OrderNo_t orderNo);
    void push_back(IntrusiveOrder &order);
    bool remove(OrderNo_t orderNo);
    std::optional<IntrusiveOrder *> remove_and_get_next(IntrusiveOrder &order) {
        auto it = order_list.iterator_to(order);
        auto next_it = std::next(it);
        remove(*it);
        if (next_it != order_list.end()) {
            return &*next_it;
        } else {
            return std::nullopt;
        }
    }
};

class IntrusivePriceLeader : public PriceLeader {
public:
    folly::IntrusiveListHook hook;
    IntrusivePriceLeader(int org_size):PriceLeader(org_size){}
};

using IntrusivePriceLeaderList = folly::IntrusiveList<IntrusivePriceLeader, &IntrusivePriceLeader::hook>;

class PriceLeaderPool {
public:
    friend IntrusivePriceLeader;
    std::vector<IntrusivePriceLeader> pl_pool;
    IntrusivePriceLeaderList free_list;// 空闲价格leader节点
    int orgs;
    PriceLeaderPool(size_t size, int org_size) : orgs(org_size) {
        pl_pool.reserve(size);
        for (int i = 0; i < size; i++) {
            pl_pool.emplace_back(org_size);
        }
        for (auto &pleader: pl_pool) {
            free_list.push_back(pleader);
        }
    }
    IntrusivePriceLeader &get_price_leader(int64_t price, std::shared_ptr<OrderPool> order_pool) {
        if (!free_list.empty()) {
            IntrusivePriceLeader &price_leader = free_list.front();
            free_list.pop_front();
            price_leader.price = price;
            price_leader.order_pool = order_pool;
            price_leader.clear();
            return price_leader;
        } else {
            throw std::runtime_error("memory exhausted");
        }
    }
    void release_price_leader(IntrusivePriceLeader &price_leader) {
        price_leader.order_list.clear();
        free_list.push_back(price_leader);
    }
};

using TickQueue = folly::ProducerConsumerQueue<TickInfo>;

template<char Side>
struct Compare {
    bool operator()(const int64_t &a, const int64_t &b) const {
        if constexpr (Side == 'S') {
            //对于sell方向，top价格是sell的最低价，因此应该是升序排列
            return a < b;// 升序排序
        } else {
            //对于buy方向，top价格是buy的最高价，因此是降序排列
            return a > b;// 降序排序
        }
    }
};

template<char Side>
using PriceLevel = absl::btree_map<int64_t, IntrusivePriceLeaderList::iterator, Compare<Side>>;


class OrderBook {
private:
    template<char Side>
    void remove_level(typename PriceLevel<Side>::iterator price_iter) {
        IntrusivePriceLeader &price_leader = *price_iter->second;
        price_leader_pool->release_price_leader(price_leader);
        if constexpr (Side == 'B') {
            buy_level.erase(price_iter);
        } else if constexpr (Side == 'S') {
            sell_level.erase(price_iter);
        } else {
            static_assert(false);
        }
    }
    template<char Side>
    void delete_order(int64_t price, OrderNo_t orderNo) {
        typename PriceLevel<Side>::iterator price_iter;
        if constexpr (Side == 'B') {
            price_iter = buy_level.find(price);
            if (price_iter == buy_level.end()) {
                throw std::runtime_error(fmt::format("failed to find price {} in buy level", price));
            }
        }else {
           price_iter = sell_level.find(price);
            if (price_iter == sell_level.end()) {
                throw std::runtime_error(fmt::format("failed to find price {} in sell level", price));
            }
        }
        price_iter->second->remove(orderNo);
        if (price_iter->second->order_list.empty()) {
            remove_level<Side>(price_iter);
        }
    }
    template<char Side>
    void delete_order(typename PriceLevel<Side>::iterator price_iter, OrderNo_t orderNo) {
        price_iter->second->remove(orderNo);
        if (price_iter->second->order_list.empty()) {
            remove_level<Side>(price_iter);
        }
    }

public:
    TickQueue tick_queue;
    PriceLevel<'B'> buy_level;
    PriceLevel<'S'> sell_level;
    IntrusivePriceLeaderList price_leaders;
    std::unique_ptr<BiCredit> bi_credit;
    std::shared_ptr<OrderPool> order_pool;
    std::shared_ptr<PriceLeaderPool> price_leader_pool;
    int org_size;
    OrderBook(size_t queue_size, size_t order_pool_size, size_t price_pool_size, int org_size)
        : tick_queue(queue_size),org_size(org_size) {
        order_pool = std::make_shared<OrderPool>(order_pool_size);
        price_leader_pool = std::make_shared<PriceLeaderPool>(price_pool_size, org_size);
    }
    void process_trade(TickInfo *tick);
    void process_order_add(TickInfo *tick);
    void process_order_cancel(TickInfo *tick);
    void display_order_book(int level);
    void display_depth(int level);
};





// void run() {
//     while (1) {
//         if (!tick_queue.isEmpty()) {
//             TickInfo *tick = tick_queue.frontPtr();
//             if (tick->type == TickInfo::Trade) {
//                 //进行成交
//             } else if (tick->type == TickInfo::OrderAdd) {
//                 IntrusiveOrder &order = order_pool->get_order();
//             } else if (tick->type == TickInfo::OrderCancel) {
//             }
//         }
//     }
// }
#endif