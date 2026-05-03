#pragma once
#include<string>
#include<iostream>
#include<mutex>
#include"MemoryBlock.h"

class MemoryPool
{
private:
	MemoryBlock* free_list_;
	std::mutex pool_mutex;
	size_t total_blocks;
	size_t free_blocks;
public:
	MemoryPool(const MemoryPool&) = delete; // Disable copy constructor
	MemoryPool& operator=(const MemoryPool&) = delete; // Disable copy assignment operator
	MemoryPool(size_t t = 64) : total_blocks(t), free_blocks(t)
	{
		free_list_ = new MemoryBlock();
		MemoryBlock* current = free_list_;
		for (int i = 1; i < t; i++)
		{
			MemoryBlock* newBlock = new MemoryBlock();
			current->setNext(newBlock);
			current = newBlock;
		}
		
	}
	MemoryBlock* allocate()
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
		return node;
	}
	void free(MemoryBlock* block)
	{
		if (block == nullptr)
		{
			std::cout << "Error: Attempt to free a null block!" << std::endl;
			return; // Invalid block
		}
		if (block->is_freed())
		{
			std::cout << "Error: Double free of memory blocks!" << "\n";
			return;
		}
		{
			std::lock_guard<std::mutex> lock(pool_mutex);
			block->set_dead();
			block->setNext(free_list_);
			free_list_ = block;
			free_blocks++;
		}
	}
	~MemoryPool()
	{
		MemoryBlock* current = free_list_;
		while (current)
		{
			MemoryBlock* next = current->getNext();
			delete current;
			current = next;
		}
		free_list_ = nullptr;
	}

	

};

