#include "order.hpp"
#include "fmt/format.h"
#include <absl/log/log.h>
#include <absl/log/absl_log.h>
void Order::fill_order_from_tick(TickInfo *tick) {
    //order的方向取决于订单的方向
    b_s = tick->b_s;
    if (tick->b_s == 'B') {
        //如果order是买方向，则取buyNo
        orderNo = tick->buy_orderNo;
        org = tick->buy_orgNo;
    } else {
        orderNo = tick->sell_orderNo;
        org = tick->sell_orgNo;
    }
    price = tick->price;
    tp = tick->timestamp;
    volume = tick->trade_money;
}