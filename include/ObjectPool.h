#pragma once

#include<typeinfo>
#include<string>
#include<utility>
#include<cstdint>
#include<iostream>

#include<cstdio>

#include<vector>//主要用来存储大内存块的起始地址

template<typename T>
class ObjectPool
{
private:
	struct Header
	{
		bool is_used = false;
		Header* next = nullptr;
		void* block_start = nullptr;//归还对象指针时方便追溯其所属的大内存区,方便is_owned()
	};
	size_t big_blocks;//有几个大内存区
	size_t sub_blocks;//不同的大内存区,子内存块数量一致吗
	size_t total_sizes;
	size_t alignas_sizes;
	size_t header_sizes;
	size_t memory_capacity;//所有大内存区的内存大小统一吗
	size_t user_need;
	static const uint64_t CANARY_VALUE = 0xDEADBEEFCAFEBABEULL;
	std::vector<void*> BigBlocks; //便于析构
	Header* free_list;

public:
	const std::type_info* type_info_type = &typeid(T);
	size_t alignas_up(size_t x, size_t alignasment)
	{
		return (x + alignasment - 1) & ~(alignasment - 1);
	}
	static const size_t type_sizes = sizeof(T);

	explicit ObjectPool(size_t tc) : sub_blocks(tc), alignas_sizes(alignof(std::max_align_t))
	{
		user_need = alignas_up(type_sizes, alignas_sizes);
		header_sizes = sizeof(Header);
		total_sizes = header_sizes + user_need + 8;//栅栏大小
		//先向操作系统申请一大块内存
		memory_capacity = total_sizes * sub_blocks;

		void* pool_ptr = operator new(memory_capacity);
		BigBlocks.emplace_back(pool_ptr);//存储大内存块的起始地址
#if defined(_DEBUG) 
		printf("start address of the big_memory_block: %p\n", pool_ptr);
#endif 
		//std::cout << "start address of the big_memory_block:" << pool_ptr << "\n";
		free_list = (Header*)pool_ptr;
		//分割内存块
		Header* current ;
		for (size_t i = 0; i < sub_blocks ; i++)
		{
			current = reinterpret_cast<Header*>((char*)(pool_ptr)+i * total_sizes);
			uint64_t* canary = (uint64_t*)((char*)(pool_ptr)+i * total_sizes + (header_sizes + user_need));
			*canary = CANARY_VALUE;
			current->is_used = false;
			current->block_start = pool_ptr;
			if(i!=sub_blocks-1)
			{
				current->next = (Header*)((char*)(pool_ptr)+(i + 1) * (total_sizes));
			}
			else//i==sub_blocks-1
			{
				//std::cout << "current->next = nullptr\n";
				current->next = nullptr;
			}
		}

	}
	//扩容
	void* allocate_new_BigBlock()
	{
		void* pool_ptr = operator new(memory_capacity);
		BigBlocks.emplace_back(pool_ptr);//存储大内存块的起始地址
		std::cout << "start address of the big_memory_block:" << pool_ptr << "\n";
		
		Header* current = reinterpret_cast<Header*>(pool_ptr);
		if (free_list) free_list->next = current;
		else free_list = current;
		//分割内存块
		for (size_t i = 0; i < sub_blocks; i++)
		{
			current = reinterpret_cast<Header*>((char*)(pool_ptr)+i * total_sizes);
			uint64_t* canary = (uint64_t*)((char*)(pool_ptr)+i * total_sizes + (header_sizes + user_need));
			*canary = CANARY_VALUE;
			current->is_used = false;
			current->block_start = pool_ptr;
			if (i != sub_blocks - 1)
			{
				current->next = (Header*)((char*)(pool_ptr)+(i + 1) * (total_sizes));
			}
			else//i==sub_blocks-1
			{
				//std::cout << "current->next = nullptr\n";
				current->next = nullptr;
			}
		}

		return pool_ptr;
	}
	/*
	void split_BigBlock(void* BigBlock_ptr)
	{

	}
	*/
	// 禁止拷贝
	ObjectPool(const ObjectPool&) = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;
		
	~ObjectPool()
	{
		//确保所有对象都已经正常销毁了
		for (auto& block_start : BigBlocks)
		{
			for (size_t i = 0; i < sub_blocks; i++)
			{
				T* user_ptr = (T*)((char*)block_start + header_sizes + i * total_sizes);
				user_ptr->~T();
			}
			operator delete(block_start);
			block_start = nullptr;
		}
		free_list = nullptr;
	}
	template<typename ... Args>
	T* acquire(Args ...args)
	{
		if (free_list == nullptr)
		{
			std::cerr << "No free memory! Expand the memory capacity.\n";
			//将来要支持自动扩容
			allocate_new_BigBlock();
			//return nullptr;
		}
		
#if defined(_DEBUG)  
		printf("The current address of free_list: %p\n", free_list);
#endif
		//std::cout << "The current address of free_list:" << free_list ;//其Header*指针地址
		free_list->is_used = true;
		char* p = (char*)free_list + header_sizes;
		free_list = free_list->next;
#if defined(_DEBUG) 
		printf("The current address of object: %p\n", static_cast<void*>(p));
#endif 
		//std::cout << " ,the address of the object:" << (void*)p <<  "\n";
		return new(p) T(std::forward<Args>(args)...);
	}

	void release(T* obj_ptr)
	{
		if (obj_ptr == nullptr)
		{
			std::cerr << "A null pointer cannot be returned!\n";
			return;
		}
		
		Header* header_ptr = (Header*)((char*)obj_ptr - header_sizes);
		char* pool_ptr_char = reinterpret_cast<char*>( header_ptr->block_start );
		//std::cout << "The address of the returned object pointer:" << header_ptr << "\n";
		if (((char*)header_ptr < pool_ptr_char) || ((char*)header_ptr > pool_ptr_char + (sub_blocks - 1) * total_sizes))
		{
			std::cerr << "The pointer address is not within a reasonable range!\n";
			return;
		}
		
		if (((char*)header_ptr - pool_ptr_char) % total_sizes != 0)
		{
			std::cerr << "The pointer address is not within a valid range!\n";
			return;
		}
		if (!header_ptr->is_used)
		{
			std::cerr << "double free!\n";
			return;//避免double free;
		}
		//检查一下栅栏，是否存在越界写入现象
		uint64_t* canary_ptr = (uint64_t*)((char*)obj_ptr + user_need);
		if (*canary_ptr != CANARY_VALUE)
		{
			std::cerr << "The canary area has been illegally written across the boundary!\n";
			return;
		}
#if defined(_DEBUG) 
		printf("The address of the free_list before return: % p\n", free_list);
#endif 
#if defined(_DEBUG) 
		printf("The current address of object: %p\n", static_cast<void*>(obj_ptr));
#endif 

		obj_ptr->~T();
#if defined(_DEBUG) 
		printf("The object destructor is called successfully.\n");
#endif
		std::memset(obj_ptr, 0xDD, user_need);
		header_ptr->next = free_list;
		free_list = header_ptr;
		header_ptr->is_used = false;
	}

	bool is_owned(T* obj_ptr)
	{
		Header* header_ptr = reinterpret_cast<Header*>((char*)obj_ptr - header_sizes);
		void* pool_ptr = header_ptr->block_start;
		if (((char*)header_ptr < (char*)pool_ptr) || ((char*)header_ptr > (char*)pool_ptr + (sub_blocks - 1) * total_sizes))
		{
			std::cerr << "The pointer address is not within a reasonable range!\n";
			return false;
		}
		if (((char*)obj_ptr - (char*)pool_ptr) % total_sizes != 0)
		{
			std::cerr << "The pointer address is not within a valid range!\n";
			return false;
		}
		return true;
	}

	Header* get_header(T* obj)
	{
		if (!obj)
		{
			std::cerr << "A null pointer cannot obtain a Header pointer!\n";
			return nullptr;
		}
		return  reinterpret_cast<Header*>((char*)obj - header_sizes);
	}

	T* get_user_ptr(Header* header_ptr)
	{
		if (!header_ptr)
		{
			std::cerr << "A null pointer cannot obtain a User pointer!\n";
			return nullptr;
		}
		return (T*)((char*)header_ptr + header_sizes);
	}

	uint64_t* get_canary_ptr(T* obj_ptr)
	{
		if (!obj_ptr)
		{
			std::cerr << "A null pointer cannot obtain a Canary pointer!\n";
			return nullptr;
		}
		return (uint64_t*)((char*)obj_ptr + user_need);
	}
};
