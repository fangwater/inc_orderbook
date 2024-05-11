#ifndef TICK_INFO_HPP
#define TICK_INFO_HPP
#include <cstdint>
#include <vector>
struct TickInfo {
    constexpr static char OrderAdd = 'A';
    constexpr static char OrderCancel = 'D';
    constexpr static char Trade = 'T';
    char type; //ADT
    char b_s;
    int64_t timestamp;
    int64_t price;
    int64_t buy_orderNo;
    int64_t sell_orderNo;
    int64_t trade_money;
    int64_t buy_orgNo;
    int64_t sell_orgNo;
};

struct IncrementOrderBookInfo {
    int64_t timestamp;
    bool is_snapshot;
    char side;
    int64_t price;
    int64_t amount;
    IncrementOrderBookInfo(int64_t _timestamp, bool _is_snapshot, char _side, float _price, float _amount)
        : timestamp(_timestamp), is_snapshot(_is_snapshot), side(_side), price(_price), amount(_amount) {}
};


class OrderBookSnapShot {
private:
    using PriceAmount = std::pair<int64_t,int64_t>;
    std::vector<PriceAmount> asks;
    std::vector<PriceAmount> bids;
    OrderBookSnapShot(int ask_depth, int bid_depth, int query_depth) : asks(ask_depth), bids(bid_depth) {

    }
    OrderBookSnapShot() = delete;

public:
    
};







#endif