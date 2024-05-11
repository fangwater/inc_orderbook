#ifndef INC_INFO_BUFFER
#define INC_INFO_BUFFER

#include "tick.hpp"
#include <folly/ProducerConsumerQueue.h>
#include <memory>

template<typename T>
using Queue = folly::ProducerConsumerQueue<T>;

template<typename T>
using QueuePtr = std::unique_ptr<Queue<T>>;

class IncrementOrderBookInfoBuffer {
public:
    void append_inc(IncrementOrderBookInfo &inc_info);
    IncrementOrderBookInfoBuffer() = delete;
    explicit IncrementOrderBookInfoBuffer(size_t buffer_size);
public:
    QueuePtr<IncrementOrderBookInfo> inc_buffer_ptr_;
};

#endif