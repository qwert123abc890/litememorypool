#pragma once
#include<iostream>
#include<cstddef>
#include<cstdint>
//const size_t MIN_SPLIT_SIZE = sizeof(BinHeader)+sizeof(uint64_t) ;
struct BinHeader
{
	BinHeader* prev;
	BinHeader* next;
	size_t size;
	bool is_used;
	BinHeader() : prev(nullptr), next(nullptr), size(0), is_used(false) {}
	BinHeader(size_t s) : prev(nullptr),next(nullptr),size(s), is_used(false)  {}
	void* user_ptr()
	{
		return (void*)(this + 1);
	}
	BinHeader* find_best_fit(size_t need)
	{
		BinHeader* current = this;
		BinHeader* best_fit = nullptr;
		while(current->next!=current)
		{
			if (!current->is_used && current->size >= need)
			{
				if (!best_fit || current->size < best_fit->size)
				{
					best_fit = current;
				}
			}
			current = current->next;
		}
		return best_fit;
	}
	BinHeader* split(size_t needed_size) //needed_size includes the size of the header
	{
		if(this->size <= needed_size)
		{
			std::cerr << "Not enough space to split\n" ;
			return nullptr;
		}
		//if (this->size - needed_size <= MIN_SPLIT_SIZE)
		if (this->size - needed_size <= 16)
		{
			return nullptr;
		}
		BinHeader* remaining_block_ptr = (BinHeader*)((char*)this + needed_size);
		remaining_block_ptr->size = this->size - needed_size;
		remaining_block_ptr->is_used = false;
		remaining_block_ptr->prev = nullptr;
		remaining_block_ptr->next = nullptr;
		this->size = needed_size;

		return remaining_block_ptr;
	}
	BinHeader* merge_with_next()
	{
		
	}
	void remove_from_list()
	{
		this->prev->next = this->next;
		this->next->prev = this->prev;

		this->prev = nullptr;
		this->next = nullptr;
		this->is_used = true;
	}
	
};
