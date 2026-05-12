#pragma once

#include<typeinfo>
#include<string>
#include<utility>
#include<cstdint>
#include<iostream>
template<typename T>
class ObjectPool
{
private:
	struct Header
	{
		bool is_used = false;
		Header* next = nullptr;
		uint64_t* canary;
	};
	size_t total_blocks;
	size_t total_sizes;
	size_t alignas_sizes;
	size_t header_sizes;
	size_t memory_capacity;
	size_t user_need;
	static const uint64_t CANARY_VALUE = 0xDEADBEEFCAFEBABEULL;
	void* pool_ptr;
	Header* free_list;
public:
	const std::type_info* type_info_type = &typeid(T);
	size_t alignas_up(size_t x, size_t alignasment)
	{
		return (x + alignasment - 1) & ~(alignasment - 1);
	}
	static const size_t type_sizes = sizeof(T);
	explicit ObjectPool(size_t tc) : total_blocks(tc), alignas_sizes(alignof(std::max_align_t))
	{
		user_need = alignas_up(type_sizes, alignas_sizes);
		header_sizes = sizeof(Header);
		total_sizes = header_sizes + user_need + 8;//栅栏大小
		//先向操作系统申请一大块内存
		memory_capacity = total_sizes * total_blocks;
		pool_ptr = operator new(memory_capacity);
		std::cout << "start address of the memory block:" << pool_ptr << "\n";
		//pool_ptr = operator new[total_sizes*total_blocks];
		free_list = (Header*)pool_ptr;
		//分割内存块
		Header* current = free_list;
		for (size_t i = 0; i < total_blocks ; i++)
		{
			current = reinterpret_cast<Header*>((char*)(pool_ptr)+i * total_sizes);
			current->canary = (uint64_t*)((char*)(pool_ptr)+i * total_sizes + (header_sizes + user_need));
			*(current->canary) = CANARY_VALUE;
			current->is_used = false;
			if (i != total_blocks - 1)
			{
				current->next = (Header*)((char*)(pool_ptr)+(i + 1) * (total_sizes));
			}
			else
			{
				current->next = nullptr;
			}
		}
		/*
		for (size_t i = 0; i < total_blocks - 1; i++)
		{
			current = (Header*)( (char*)(pool_ptr)+i * total_sizes );
			current->canary = (uint64_t*)((char*)(pool_ptr)+i * total_sizes + (header_sizes + user_need));
			//在栅栏处写入
			*(current->canary) = CANARY_VALUE;
			current->next = (Header*)( (char*)(pool_ptr)+(i + 1) * (total_sizes) );
		}
		*/
		

	}
	~ObjectPool()
	{
		//确保所有对象都已经正常销毁了
		//T* user_ptr = (T*)((char*)free_list + header_sizes);
		for (size_t i = 0; i < total_blocks; i++)
		{
			T* user_ptr = (T*)((char*)free_list + header_sizes + i*total_sizes);
			user_ptr->~T();
		}
		operator delete(pool_ptr);
		pool_ptr = nullptr;
		free_list = nullptr;
	}
	template<typename ... Args>
	T* acquire(Args ...args)
	{
		if (free_list == nullptr)
		{
			std::cout << "No free memory!\n";
			//将来要支持自动扩容
			return nullptr;
		}
		std::cout << "The current address of free_list:" << free_list << "\n";//其Header*指针地址
		free_list->is_used = true;
		char* p = (char*)free_list + header_sizes;
		free_list = free_list->next;
		//不太会写
		return new(p) T(std::forward<Args>(args)...);
	}

	void release(T* obj_ptr)
	{
		if (obj_ptr == nullptr)
		{
			std::cout << "A null pointer cannot be returned!\n";
			return;
		}
		//if (!is_owned(obj_ptr)) return;
		Header* header_ptr = (Header*)((char*)obj_ptr - header_sizes);
		//std::cout << "The address of the returned object pointer:" << header_ptr << "\n";
		if (((char*)header_ptr < (char*)pool_ptr) || ((char*)header_ptr > (char*)pool_ptr + (total_blocks - 1) * total_sizes))
		{
			std::cout << "The pointer address is not within a reasonable range!\n";
			return;
		}
		//if (((char*)obj_ptr - (char*)pool_ptr) % total_sizes != 0)
		if (((char*)header_ptr - (char*)pool_ptr) % total_sizes != 0)
		{
			std::cout << "The pointer address is not within a valid range!\n";
			return;
		}
		if (!header_ptr->is_used)
		{
			std::cout << "double free!\n";
			return;//避免double free;
		}
		//检查一下栅栏，是否存在越界写入现象
		uint64_t* canary_ptr = (uint64_t*)((char*)obj_ptr + user_need);
		if (*canary_ptr != CANARY_VALUE)
		{
			std::cout << "The canary area has been illegally written across the boundary!\n";
			return;
		}
		obj_ptr->~T();
		header_ptr->next = free_list;
		free_list = header_ptr;
		header_ptr->is_used = false;
	}

	bool is_owned(T* obj_ptr)
	{
		Header* header_ptr = reinterpret_cast<Header*>( (char*)obj_ptr - header_sizes );
		if (((char*)header_ptr < (char*)pool_ptr) || ((char*)header_ptr > (char*)pool_ptr + (total_blocks - 1) * total_sizes))
		{
			std::cout << "The pointer address is not within a reasonable range!\n";
			return false;
		}
		if (((char*)obj_ptr - (char*)pool_ptr) % total_sizes != 0)
		{
			std::cout << "The pointer address is not within a valid range!\n";
			return false;
		}
		return true;
	}

	Header* get_header(T* obj)
	{
		if (!obj)
		{
			std::cout << "A null pointer cannot obtain a Header pointer!\n";
			return nullptr;
		}
		return  reinterpret_cast<Header*>( (char*)obj - header_sizes );
	}

	T* get_user_ptr(Header* header_ptr)
	{
		if (!header_ptr)
		{
			std::cout << "A null pointer cannot obtain a User pointer!\n";
			return nullptr;
		}
		return (T*)((char*)header_ptr + header_sizes);
	}

	uint64_t* get_canary_ptr(T* obj_ptr)
	{
		if (!obj_ptr)
		{
			std::cout << "A null pointer cannot obtain a Canary pointer!\n";
			return;
		}
		return (uint64_t*)((char*)obj_ptr + user_need);
	}
};
