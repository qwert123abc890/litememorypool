#pragma once

#include<typeinfo>
#include<string>
#include<utility>
#include<iostream>
template<typename T>
class ObjectPool
{
private:
	struct Header
	{
		bool is_used = false;
		Header* next = nullptr;
		char* canary;
	};
	size_t total_counts;
	size_t total_sizes;
	size_t alignas_sizes;
	size_t header_sizes;
	size_t memory_capacity;
	size_t user_need;
	void* pool_ptr;
	Header* free_list;
public:
	const std::type_info* type_info_type = &typeid(T);
	size_t alignas_up(size_t x, size_t alignasment)
	{
		return (x + alignasment - 1) & ~(alignasment - 1);
	}
	static const size_t type_sizes = sizeof(T);
	ObjectPool(const std::type_info* ti, size_t tc) : type_info_type(ti), total_counts(tc), alignas_sizes(alignof(std::max_align_t))
	{
		user_need = alignas_up(type_sizes, alignas_sizes);
		total_sizes = sizeof(Header) + user_need + 8;
		header_sizes = sizeof(Header);
		//先向操作系统申请一大块内存
		memory_capacity = total_sizes * total_counts;
		pool_ptr = operator new(memory_capacity);
		//pool_ptr = operator new[total_sizes*total_counts];
		free_list = (Header*)pool_ptr;
		//分割内存块
		Header* current = free_list;
		for (size_t i = 0; i < total_counts - 1; i++)
		{

			current = (Header*)( (char*)(pool_ptr)+i * total_sizes );
			current->canary = (char*)(pool_ptr)+i * total_sizes + (header_sizes + user_need);
			//在栅栏处写入
			current->next = (Header*)( (char*)(pool_ptr)+(i + 1) * (total_sizes) );
		}

		//for (size_t i = 0; i < total_counts; i++)
		//{
		//	if (i != total_counts - 1)
		//	{
		//		(Header*)current = (char*)(pool_ptr)+i*total_sizes;
		//		current->next = (char*)(pool_ptr)+(i + 1) * (total_sizes);
		//	}
		//	else
		//	{
		//		current->next = nullptr;
		//	}
		//}


	}
	~ObjectPool()
	{
		//确保所有对象都已经正常销毁了
		//T* user_ptr = (T*)((char*)free_list + header_sizes);
		for (size_t i = 0; i < total_counts; i++)
		{
			T* user_ptr = (T*)((char*)free_list + header_sizes + i*total_sizes);
			user_ptr->~T();
		}
		operator delete(pool_ptr;
		pool_ptr = nullptr;
		free_list = nullptr;
	}
	template<typename ... Args>
	T* acquire(Args ...args)
	{
		if (free_list == nullptr) return nullptr;
		std::cout << free_list << "\n";
		free_list->is_used = true;
		char* p = (char*)free_list + header_sizes;
		free_list = free_list->next;
		return new(p) T(std::forward<Args>(args)...);
	}

	void release(T* obj_ptr)
	{
		if (obj_ptr == nullptr) return;
		//if (!is_owned(obj_ptr)) return;
		Header* header_ptr = (Header*)((char*)obj_ptr - header_sizes);
		if ( ((char*)header_ptr < (char*)pool_ptr) || ((char*)header_ptr >= (char*)pool_ptr + (total_counts - 1) * total_sizes) ) return ;
		if (( (char*)obj_ptr - (char*)pool_ptr ) % total_sizes != 0) return ;
		if(!header_ptr->is_used) return ;//避免double free;
		//检查一下栅栏，是否存在越界写入现象
		obj_ptr->~T();
		header_ptr->next = free_list;
		free_list = header_ptr;
		header_ptr->is_used = false;
	}

	bool is_owned(T* obj_ptr)
	{
		Header* header_ptr = (char*)obj_ptr - header_sizes;
		if (((char*)header_ptr < (char*)pool_ptr) || ((char*)header_ptr >= (char*)pool_ptr + (total_counts - 1) * total_sizes)) return false;
		if (( (char*)obj_ptr - (char*)pool_ptr ) % total_sizes != 0) return false;
		return true;
	}

	Header* get_header(T* obj)
	{
		if (!obj) return nullptr;
		return  (char*)obj - header_sizes;
	}

	T* get_user_ptr(Header* header_ptr)
	{
		if (!header_ptr) return nullptr;
		return (T*)((char*)header_ptr + header_sizes);
	}
	
};
