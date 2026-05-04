#pragma once
#include<string>
#include<iostream>
#include<mutex>
#include"MemoryBlock.h"

class MemoryPool
{
private:
	MemoryBlock* free_list_;
	void* pool_memory_;
	std::mutex pool_mutex;
	size_t total_blocks;
	size_t free_blocks;
	size_t user_data_size;
	size_t header_size;
	size_t total_size;
private:
	size_t align_up(size_t x, size_t align)
    {
        return (x + align - 1) & ~(align - 1);
    }
public:
	MemoryPool(const MemoryPool&) = delete; // Disable copy constructor
	MemoryPool& operator=(const MemoryPool&) = delete; // Disable copy assignment operator
	explicit MemoryPool(size_t t = 64) : total_blocks(t), free_blocks(t) //避免隐式类型转换
	{
		header_size = align_up( sizeof(MemoryBlock),alignof(std::max_align_t) );
		total_size = header_size*total_blocks;

		pool_memory_ = ::operator new (total_size);
		free_list_ = (MemoryBlock*)pool_memory_;
	}
	void* allocate()
	{
		std::lock_guard<std::mutex> lock(pool_mutex);
		if (!free_list_)
		{
			std::cout << "Error: No blocks available for allocation!" << std::endl;
			return nullptr; // No blocks available
		}

		MemoryBlock* node = free_list_;
		free_list_ = free_list_->getNext();
		free_blocks--;
		node->setNext(nullptr); // Detach the allocated block from the pool
		node->set_alive();
		return (char*)node + sizeof(MemoryBlock);
	}
	void free()
	{
		
	}
	~MemoryPool()
	{
		MemoryBlock* current = free_list_;
		while(current)
		{
			free_list_ = current->getNext();
			current->~MemoryBlock();
			current = free_list_;
		}
		delete[] pool_memory_;
		free_list_ = nullptr;
		pool_memory_ = nullptr;
	}
	bool is_owned(void* ptr)
	{
		return ((char*)ptr >= (char*)pool_memory_ + header_size) && ((char*)ptr <= (char*)pool_memory_ + total_size) ;
	}
};

