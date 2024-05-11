// #include "orderbook.hpp"
// #include "tick.hpp"
// #include <iostream>
// #include <sstream>
// #include <string>
// #include <vector>
// std::vector<TickInfo> make_test_tick() {
//     std::vector<TickInfo> ticks;
//     for (int i = 4; i < 7; i++) {
//         TickInfo t;
//         t.type = TickInfo::OrderAdd;
//         t.price = i;
//         t.b_s = 'S';
//         t.sell_orderNo = i;
//         t.trade_money = i * 100;
//         t.buy_orgNo = 123;
//         ticks.push_back(t);
//     }
//     for (int i = 1; i < 4; i++) {
//         TickInfo t;
//         t.type = TickInfo::OrderAdd;
//         t.price = i;
//         t.b_s = 'B';
//         t.buy_orderNo = i;
//         t.trade_money = i * 100;
//         t.sell_orgNo = 234;
//         ticks.push_back(t);
//     }
//     return ticks;
// }

// int main() {
//     auto ticks = make_test_tick();
//     OrderBook orderbook(1000, 1000, 1000);
//     for (auto &tick: ticks) {
//         orderbook.process_order_add(&tick);
//     }
//     orderbook.display_depth(3);
//     printf("============================\n");
//     TickInfo t;
//     t.type = TickInfo::OrderAdd;
//     t.price = 4;
//     t.b_s = 'B';
//     t.trade_money = 400;
//     t.sell_orgNo = 234;
//     t.buy_orderNo = 4;
//     orderbook.process_order_add(&t);
//     orderbook.display_depth(5);
//     printf("============================\n");
//     t.type = TickInfo::Trade;
//     t.price = 4;
//     t.b_s = 'B';
//     t.buy_orderNo = 4;
//     t.sell_orderNo = 4;
//     t.trade_money = 200;
//     orderbook.process_trade(&t);
//     orderbook.display_depth(3);
//     printf("============================");
//     return 0;
// }
#include <chrono>
#include <iostream>
#include <vector>
#include "orderbook.hpp"
#include "tick.hpp"
std::vector<TickInfo> generate_test_data(int count, char type) {
    std::vector<TickInfo> ticks;
    ticks.reserve(count);
    for (int i = 0; i < count; ++i) {
        TickInfo tick;
        tick.type = TickInfo::OrderAdd;
        tick.price = (type == 'B' ? 100 + i : 100 - i);
        tick.b_s = type;
        tick.trade_money = 100 * (i + 1);
        tick.buy_orderNo = i + 1;
        tick.sell_orderNo = i + 1;
        ticks.push_back(tick);
    }
    return ticks;
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    auto buy_ticks = generate_test_data(300, 'B');
    auto sell_ticks = generate_test_data(300, 'S');
    OrderBook orderbook(1000, 1000, 1000,100);

    for (auto &tick: buy_ticks) {
        orderbook.process_order_add(&tick);
    }
    for (auto &tick: sell_ticks) {
        orderbook.process_order_add(&tick);
    }

    orderbook.display_depth(5);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "Total time taken: " << elapsed.count() << " ms\n";

    return 0;
}