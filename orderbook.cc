#include "orderbook.hpp"
#include "order.hpp"
#include "tick.hpp"
#include <cstdint>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
void PriceLeader::remove(IntrusiveOrder &order) {
    vol -= order.volume;
    order_list.remove(order);
    if (order_pool) {
        order_pool->recycle_order(order);
    } else {
        throw std::runtime_error("order pool is not bind");
    }
}
std::optional<SparseMatrixElement> OrgContribution::get_contribution_item(int from, int to) {
    auto iter = contribution_volumes_.find({from, to});
    if (iter != contribution_volumes_.end()) {
        return iter;
    } else {
        return std::nullopt;
    }
}

PriceLeader::PriceLeader(int orgs) {
    org_aggregated_vol = std::make_unique<std::vector<int64_t>>(orgs);
    org_executable_vols = std::make_unique<std::vector<int64_t>>(orgs);
    org_contribution = std::make_unique<OrgContribution>();
}

void PriceLeader::clear() {
    executable_vol = 0;
    org_contribution->contribution_volumes_.clear();
    std::fill(org_aggregated_vol->begin(), org_aggregated_vol->end(), 0);
    std::fill(org_executable_vols->begin(), org_executable_vols->end(), 0);
}

void PriceLeader::push_back(IntrusiveOrder &order) {
    vol += order.volume;
    order_list.push_back(order);
}

bool PriceLeader::remove(OrderNo_t orderNo) {
    for (auto iter = order_list.begin(); iter != order_list.end(); iter++) {
        if (iter->orderNo == orderNo) {
            //find order
            remove(*iter);
            return true;
        }
    }
    return false;
}

std::optional<IntrusiveOrder *> PriceLeader::find_order(OrderNo_t orderNo) {
    for (auto iter = order_list.begin(); iter != order_list.end(); iter++) {
        if (iter->orderNo == orderNo) {
            return &*iter;
        } else {
            LOG(INFO) << fmt::format("OrderNo is {}", iter->orderNo);
        }
    }
    return std::nullopt;
}

void OrderBook::display_order_book(int level) {
    auto biter = buy_level.begin();
    int count = 0;
    std::string ob_str;
    ob_str += "-----------------\n|buy\n-----------------\n";
    std::stack<std::string> buy_levels;
    while (biter != buy_level.end() && count < level) {
        std::string line = fmt::format("{:<8}", biter->first);
        for (auto iter = biter->second->order_list.begin(); iter != biter->second->order_list.end(); iter++) {
            //打印订单内容(机构-量)
            line += fmt::format("({},{})-->", iter->org, iter->volume);
        }
        ++biter;//递增迭代器
        ++count;
        buy_levels.push(std::move(line));
    }
    while (!buy_levels.empty()) {
        ob_str += (buy_levels.top() + "\n");
        buy_levels.pop();
    }
    // 添加分隔线
    ob_str += "-----------------\n";

    ob_str += "-----------------\n|sell\n-----------------\n";
    // sell_level的处理
    auto siter = sell_level.begin();
    count = 0;
    while (siter != sell_level.end() && count < level) {
        std::string line = fmt::format("{:<8}", siter->first);
        for (auto iter = siter->second->order_list.begin(); iter != siter->second->order_list.end(); iter++) {
            //打印订单内容(机构-量)
            line += fmt::format("({},{})-->", iter->org, iter->volume);
        }
        ob_str += (line + "\n");
        ++siter;// 递增迭代器
        ++count;
    }
    ob_str += "-----------------\n\n";
    // 最终输出到控制台
    fmt::print("{}", ob_str);
}


void OrderBook::display_depth(int level) {
    std::vector<int64_t> buy_price;
    std::vector<int64_t> buy_vol;
    std::vector<int64_t> sell_price;
    std::vector<int64_t> sell_vol;

    auto biter = buy_level.begin();
    int count = 0;
    while (biter != buy_level.end() && count < level) {
        buy_price.push_back(biter->first);
        buy_vol.push_back(biter->second->vol);
        ++biter;//递增迭代器
        ++count;
    }

    // 打印买单信息，buy价格迭代自高向低，但打印过程需要反过来
    std::string ob_str;
    ob_str += "-----------------\n|buy\n-----------------\n";
    for (int i = count - 1; i >= 0; i--) {
        ob_str += fmt::format("{:<8} {:<8}\n", buy_price[i], buy_vol[i]);
    }
    // 添加分隔线
    ob_str += "-----------------\n\n";
    ob_str += "-----------------\n|sell\n-----------------\n";
    // sell_level的处理
    auto siter = sell_level.begin();
    count = 0;
    while (siter != sell_level.end() && count < level) {
        sell_price.push_back(siter->first);
        sell_vol.push_back(siter->second->vol);
        ++siter;// 递增迭代器
        ++count;
    }
    // 打印卖单信息
    for (int i = 0; i < count; i++) {
        ob_str += fmt::format("{:<8} {:<8}\n", sell_price[i], sell_vol[i]);// 左对齐
    }
    ob_str += "-----------------\n\n";
    // 最终输出到控制台
    fmt::print("{}", ob_str);
}
void OrderBook::process_order_cancel(TickInfo *tick) {
    if (tick->b_s == 'B') {
        delete_order<'B'>(tick->price, tick->buy_orderNo);
    } else {
        delete_order<'S'>(tick->price, tick->sell_orderNo);
    }
}

void OrderBook::process_trade(TickInfo *tick) {
    //处理成交消息
    OrderNo_t buyNo = tick->buy_orderNo;
    OrderNo_t sellNo = tick->sell_orderNo;
    auto b_iter = buy_level.find(tick->price);
    auto s_iter = sell_level.find(tick->price);
    if (b_iter == buy_level.end()) {
        throw std::runtime_error("buy price out of level!");
    }
    if (s_iter == sell_level.end()) {
        throw std::runtime_error("sell price out of level!");
    }
    //根据成交方向判断
    //这里假设即时成交的订单也会先提供OrderAdd，所以bs双边的No都一定可以查询到
    auto buy_order = [&b_iter, buyNo]() {
        auto res = b_iter->second->find_order(buyNo);
        if (res.has_value()) {
            return res.value();
        } else {
            throw std::runtime_error(fmt::format("can not find buyNo: {}", buyNo));
        }
    }();

    auto sell_order = [&s_iter, sellNo]() {
        auto res = s_iter->second->find_order(sellNo);
        if (res.has_value()) {
            return res.value();
        } else {
            throw std::runtime_error(fmt::format("Cannot find sellNo: {}", sellNo));
        }
    }();
    
    if (buy_order->volume == tick->trade_money) {
        //完全成交，删除单子
        delete_order<'B'>(b_iter, tick->buy_orderNo);
    } else if (buy_order->volume > tick->trade_money) {
        buy_order->volume -= tick->trade_money;
    } else {
        throw std::runtime_error("trade vol is larger than order vol");
    }
    //对sell进行相同的操作
    if (sell_order->volume == tick->trade_money) {
        delete_order<'S'>(s_iter, tick->sell_orderNo);
    } else if (sell_order->volume > tick->trade_money) {
        sell_order->volume -= tick->trade_money;
    } else {
        throw std::runtime_error("trade vol is larger than order vol");
    }
}

/**
 * @brief order add 增加一笔Order，同时建立在考虑授信的基础上
 * 
 * @param tick 
 */
void OrderBook::process_order_add(TickInfo *tick) {
    IntrusiveOrder &order = order_pool->get_order();
    order.fill_order_from_tick(tick);
    if (order.b_s == 'B') {
        auto price_iter = buy_level.find(order.price);
        if (price_iter == buy_level.end()) {
            //当前价格不存在orderbook中，需要新增一个价格节点
            IntrusivePriceLeader &price_leader = price_leader_pool->get_price_leader(order.price, order_pool);
            //此笔order作为节点的第一个订单
            price_leader.push_back(order);
            price_leaders.push_back(price_leader);
            auto last_elem_iter = --price_leaders.end();
            std::pair<int64_t, IntrusivePriceLeaderList::iterator> p(price_leader.price, last_elem_iter);
            DLOG(INFO) << fmt::format("OrderNo: {}",order.orderNo);
            buy_level.emplace(std::move(p));
            //处理行情的修正
            /**
             * @brief 
             * 1、公有行情触发增量 增加总的vol_sum
             * 2、更新对私有行情的贡献量
             */
            price_leader.executable_vol += order.volume;
            for (auto i = 0; i < org_size; i++) {
                //若机构间没有授信，则直接跳过
                int64_t c_1 = bi_credit->get<'B'>(order.org, i);
                if (!c_1)
                    continue;
                int64_t c_2 = bi_credit->get<'S'>(i, order.org);
                if (!c_2)
                    continue;
                //若机构间有授信，则查看之前的情况
                int64_t vol_min = 
            }
        } else {
            auto &price_leader = price_iter->second;
            price_leader->push_back(order);
        }
    } else {
        auto price_iter = sell_level.find(order.price);
        if (price_iter == sell_level.end()) {
            //当前价格不存在orderbook中，需要新增一个价格节点
            IntrusivePriceLeader &price_leader = price_leader_pool->get_price_leader(order.price, order_pool);
            //此笔order作为节点的第一个订单
            price_leader.push_back(order);
            price_leaders.push_back(price_leader);
            auto last_elem_iter = --price_leaders.end();
            std::pair<int64_t, IntrusivePriceLeaderList::iterator> p(price_leader.price, last_elem_iter);
            sell_level.emplace(std::move(p));
        } else {
            auto &price_leader = price_iter->second;
            price_leader->push_back(order);
        }
    }
}