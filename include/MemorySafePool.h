#pragma once
#include <cstddef>
#include <mutex>
#include <iostream>
#include <new>
#include <cstdint>
#include <cstring>
class MemorySafePool
{
private:
    struct Header
    {
        Header* next;
        bool freed;
    };

private:
    static constexpr std::uint64_t CANARY_VALUE = 0xDEADBEEFCAFEBABEULL;
    size_t canary_size_ = 0;

    Header* free_list_ = nullptr;
    std::mutex pool_mutex_;

    void* pool_memory_ = nullptr;

    size_t total_blocks_ = 0;      // 总块数
    size_t free_blocks_ = 0;       // 当前空闲块数

    size_t user_size_ = 0;         // 用户要求的每块数据区大小
    size_t align_ = 0;             // 对齐要求，通常是 alignof(std::max_align_t)

    size_t header_size_ = 0;       // Header 对齐后的大小
    size_t data_size_ = 0;         // 用户数据区对齐后的大小
    size_t block_size_ = 0;        // 每个完整 block 的大小 = header_size_ + data_size_ + canary_size_
    size_t total_size_ = 0;        // 整个内存池总字节数 = block_size_ * total_blocks_

private:
    static size_t align_up(size_t x, size_t align)
    {
        return (x + align - 1) & ~(align - 1);
    }

public:
    MemorySafePool(const MemorySafePool&) = delete;
    MemorySafePool& operator=(const MemorySafePool&) = delete;

    explicit MemorySafePool(size_t block_count = 64, size_t user_size = 128)
        : total_blocks_(block_count),
        free_blocks_(block_count),
        user_size_(user_size),
        align_(alignof(std::max_align_t))
    {
        header_size_ = align_up(sizeof(Header), align_);
        data_size_ = align_up(user_size_, align_);
        canary_size_ = align_up(sizeof(uint16_t), align_);
        block_size_ = header_size_ + data_size_ + canary_size_;
        total_size_ = block_size_ * total_blocks_;


        pool_memory_ = ::operator new(total_size_);

        char* base = static_cast<char*>(pool_memory_);

        for (size_t i = 0; i < total_blocks_; ++i)
        {
            char* block_start = base + i * block_size_;

            Header* header = reinterpret_cast<Header*>(block_start);
            header->freed = true;

            if (i + 1 < total_blocks_)
            {
                header->next = reinterpret_cast<Header*>(
                    base + (i + 1) * block_size_
                    );
            }
            else
            {
                header->next = nullptr;
            }
        }

        free_list_ = reinterpret_cast<Header*>(base);
    }

    void* allocate()
    {
        std::lock_guard<std::mutex> lock(pool_mutex_);

        if (!free_list_)
        {
            std::cout << "Error: no free block available\n";
            return nullptr;
        }

        Header* header = free_list_;
        free_list_ = header->next;

        header->next = nullptr;
        header->freed = false;

        --free_blocks_;

        void* user_ptr = get_user_ptr(header);

        std::memset(user_ptr, 0xCD, data_size_);

        std::uint64_t* canary = get_canary_ptr(user_ptr);
        *canary = CANARY_VALUE;

        return user_ptr;

    }

    void free(void* ptr)
    {
        if (!ptr)
        {
            std::cout << "Error: attempt to free nullptr\n";
            return;
        }

        if (!is_owned(ptr))
        {
            std::cout << "Error: pointer does not belong to this MemorySafePool\n";
            return;
        }

        Header* header = get_header(ptr);

        if (header->freed)
        {
            std::cout << "Error: double free detected\n";
            return;
        }

        std::uint64_t* canary = get_canary_ptr(ptr);

        if (*canary != CANARY_VALUE)
        {
            std::cout << "Error: buffer overflow detected\n";
            return;
        }

        std::memset(ptr, 0xDD, data_size_);

        std::lock_guard<std::mutex> lock(pool_mutex_);

        header->freed = true;
        header->next = free_list_;
        free_list_ = header;

        ++free_blocks_;
    }

    bool is_owned(void* ptr) const
    {
        char* base = static_cast<char*>(pool_memory_);
        char* end = base + total_size_;
        char* p = static_cast<char*>(ptr);

        if (p < base || p >= end)
        {
            return false;
        }

        size_t offset = static_cast<size_t>(p - base);
        size_t inner_offset = offset % block_size_;

        return inner_offset == header_size_;
    }

    Header* get_header(void* ptr) const
    {
        char* p = static_cast<char*>(ptr);
        return reinterpret_cast<Header*>(p - header_size_);
    }

    size_t get_total_blocks() const
    {
        return total_blocks_;
    }

    size_t get_header_size() const
    {
        return header_size_;
    }

    size_t get_free_blocks() const
    {
        return free_blocks_;
    }

    size_t get_user_size() const
    {
        return user_size_;
    }

    size_t get_block_size() const
    {
        return block_size_;
    }

    ~MemorySafePool()
    {
        ::operator delete(pool_memory_);

        pool_memory_ = nullptr;
        free_list_ = nullptr;
    }

    void* get_user_ptr(Header* header) const
    {
        return reinterpret_cast<char*>(header) + header_size_;
    }

    std::uint64_t* get_canary_ptr(void* user_ptr) const
    {
        return reinterpret_cast<std::uint64_t*>( static_cast<char*>(user_ptr) + data_size_ );
    }
};
