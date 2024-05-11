#ifndef CREDIT_HPP
#define CREDIT_HPP

/**
 * @brief 存储对手方和本方在买卖两个方向的额度 类似于区分拆入和拆出
 * 
 * 需要提供额度的更新接口和额度的访问接口，
 */
#include <cstdint>
#include <memory>
#include <vector>
using Credit_t = int64_t;

class BiCredit {
public:
    //额度考虑双边授信
    template<char Side>
    Credit_t get(int from, int to);

    template<char Side>
    void set(int from, int to, Credit_t credit);

public:
    int orgs;                                               // 机构数
    std::unique_ptr<std::vector<Credit_t>> buy_credit_data; //主动买入额度矩阵
    std::unique_ptr<std::vector<Credit_t>> sell_credit_data;//主动卖出额度矩阵
};


template<char Side>
Credit_t BiCredit::get(int from, int to) {
    int64_t index = from * orgs + to;
    if constexpr (Side == 'B') {
        //查询 from 向 to 的 买入额度
        return (*buy_credit_data)[index];
    } else if constexpr (Side == 'S') {
        //查询 from 向 to 的 卖出额度
        return (*sell_credit_data)[index];
    } else {
        static_assert(false);
    }
}

template<char Side>
void BiCredit::set(int from, int to, Credit_t credit) {
    int64_t index = from * orgs + to;
    if constexpr (Side == 'B') {
        //设置 from 向 to 的 买入额度
        (*buy_credit_data)[index] = credit;
    } else if constexpr (Side == 'S') {
        //设置 from 向 to 的 卖出额度
        (*sell_credit_data)[index] = credit;
    } else {
        static_assert(false);
    }
}

/**
 * @brief 可成交量计算 
 * 在某个价格档位上，查询当前额度的限制，需要知道当前订单薄的方向
 * 若当前为Buy方向的订单薄，为机构A计算行情，价格档报价来自B
 * 则相当于B是买方，A为卖方
 * 先获取A对B 可以卖出的额度，即get<'S'>(A_index,B_index)
 * 再获取B对A 再获取B可以买入的额度，即get<'S'>(B_index, A_index)
 * 
 * 最后
 */

/**
 * @brief 增量更新
 * 对于每个价格档位，维护一个哈希表，其格式位
 * <int,int,int64_t>(org_a, org_b, vol_sum) 
 * 含义为机构a对于机构b，在当前价位下，其在额度限制下的可成交量
 * 由于大部分机构只看行情不报价，因此不会有这个报价的item
 * 具体的流程为:
 * 对于B/S的订单薄，当触发修改时候，以B为例，收到一笔来自机构A的报价:
 * 1、OrderAdd
 * (1)此时，公有行情触发增量，增加总的vol_sum 
 * (2)更新对私有行情的贡献量，此时A提交的是一笔买单，则迭代所有当前处于订阅状态的机构:
 * 假设当前看行情的机构为B:
 * 先获取B对A 可以卖出的额度，即c1 = get<'S'>(B_index, A_index)
 * 再获取A对B 可以买入的额度，即c2 = get<'B'>(A_index, B_index)
 * 查询报价机构A对于B的行情贡献量，看是空或者之前有记录 <int,int,int64_t>(org_A, org_B, vol_sum_p) 
 * 此时，取min(vol_sum_p, c1, c2)更新vol_sum_p
 *
 *
 * 
 * 2、OrderDel
 * (1)公有行情的vol_sum扣除这一笔增量
 * (2)迭代更新所有处于订阅状态的机构:
 * 假设当前看行情的机构为B:
 * 先获取B对A 可以卖出的额度，即c1 = get<'S'>(B_index, A_index)
 * 再获取A对B 可以买入的额度，即c2 = get<'B'>(A_index, B_index)
 * 查询报价机构A对于B的行情贡献量，看是空或者之前有记录 <int,int,int64_t>(org_A, org_B, vol_commit) 
 * 查询报价机构A对于B的总报价量，vol_sum

 * (1)从vol_sum中，扣除这一笔，得到vol_sum'
 * (2)重新和c1 c2 取小，获取新的可成交量vol_commit' = min(c1, c2, vol_sum')
 * (3)计算vol_commit'和vol_commit的差值beta
 * (4)更新vol-commit为vol-commit'
 * (5)从vol_sum_i中扣除beta

 * 3、OrderChange
 * 区分OrderChange是增加某个订单的Vol 还是减少某个订单的Vol 等价于 OrderAdd或者OrderDel
 */


#endif