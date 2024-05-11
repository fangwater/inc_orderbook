#include "increment_info_buffer.hpp"
#include <stdexcept>
IncrementOrderBookInfoBuffer::IncrementOrderBookInfoBuffer(size_t buffer_size) {
    inc_buffer_ptr_ = std::make_unique<Queue<IncrementOrderBookInfo>>(buffer_size);
}


void IncrementOrderBookInfoBuffer::append_inc(IncrementOrderBookInfo &inc_info) {
    if (inc_buffer_ptr_->isFull()) {
        throw std::runtime_error("inc_buffer_ptr is full");
    }
    inc_buffer_ptr_->write(inc_info);
}
