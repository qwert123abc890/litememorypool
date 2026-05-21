#pragma once
#include<vector>
#include<cstddef>
#include"BinHeader.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
class BinManager
{
private:
	struct OSPageRecord
	{
		void* address;
		size_t size;
	};
	std::vector<OSPageRecord> os_pages_record; // 账本：记录所有从 OS 批发出来的首地址
	BinHeader virtual_heads[65];
	//最小内存块大小为32字节,其中数据区字节大小为16字节，最大内存块的数据区字节大小为1024字节,总大小为1040字节，序号为63，超过1040字节的内存块则使用mmap内存映射
	//不过还需要进一步细分，32字节的内存块被划分为fast_bin,不需要合并
	//small_bin
	//large_bin
	int get_bin_index(size_t size)
	{
		if (size > 1040) return 64; //超过1040字节的内存块则使用mmap内存映射
		return static_cast<int>((size + 15) / 16) - 2;
	}


	void* request_from_os(size_t need_size)
	{
		// 无论用户要多少，底层一律以 4KB 物理页为基本单位向操作系统批发
		size_t page_size = (need_size + 4095) & ~4095;

		#ifdef _WIN32
				// Windows 的原生批发接口
				void* mem = VirtualAlloc(nullptr, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
				if (mem) os_pages_record.push_back({ mem,page_size }); // 记账
				return mem;
		#else
				// Linux/MacOS 的原生批发接口
				void* mem = mmap(nullptr, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (mem == MAP_FAILED) return nullptr;
				os_pages_record.push_back({ mem,page_size }); // 记账
				return mem;
		#endif
	}
	void insert_into_bin(BinHeader* ptr) //确保munmap的内存块不被插入到bin中
	{
		size_t bin_size = ptr->size;
		int bin_index = get_bin_index(bin_size);
		BinHeader* head = &virtual_heads[bin_index];
		ptr->next = head->next;
		ptr->prev = head;
		head->next->prev = ptr;
		head->next = ptr;
	}
public:
	size_t alignas_up(size_t x, size_t alignasment)
	{
		return (x + alignasment - 1) & ~(alignasment - 1);
	}

	BinManager()
	{
		for (int i = 0; i <= 64; ++i)
		{
			virtual_heads[i].prev = &virtual_heads[i];
			virtual_heads[i].next = &virtual_heads[i];
			virtual_heads[i].size = 0;
			virtual_heads[i].is_used = true;
		}
	}
	void* allocate(size_t user_need)
	{
		size_t need = sizeof(BinHeader) + alignas_up(user_need, alignof(std::max_align_t)) + sizeof(uint64_t);
		int bin_index = get_bin_index(need);

		//发现没有合适的内存块，继续向下一个bin寻找，直到找到合适的内存块或者所有bin都找完了
		for (int i = bin_index; i < 64; ++i)
		{
			BinHeader* head = &virtual_heads[i];
			if (head->next == head)
			{
				continue;
			}
			BinHeader* ptr = head->next;
			ptr->remove_from_list();
			BinHeader* remaining_ptr = ptr->split(need);
			if (remaining_ptr) insert_into_bin(remaining_ptr);

			uint64_t* canary = reinterpret_cast<uint64_t*>(reinterpret_cast<char*>(ptr->user_ptr()) + user_need);
			*canary = 0xDEADBEEFCAFEBABEULL;

			return ptr->user_ptr();

		}

		BinHeader* best_fit = virtual_heads[64].find_best_fit(need);
		if (best_fit)
		{
			best_fit->remove_from_list();
			BinHeader* remaining_block_ptr = best_fit->split(need);
			if (remaining_block_ptr)
			{
				insert_into_bin(remaining_block_ptr);
			}
			best_fit->is_used = true;

			uint64_t* canary = reinterpret_cast<uint64_t*>(reinterpret_cast<char*>(best_fit->user_ptr()) + user_need);
			*canary = 0xDEADBEEFCAFEBABEULL;

			return best_fit->user_ptr();
		}
		//所有bin都找完了，还是没有合适的内存块，向操作系统批发一个4KB的内存页

		void* os_page = request_from_os(need);
		if (!os_page) return nullptr;

		BinHeader* header_ptr = reinterpret_cast<BinHeader*>(os_page);
		header_ptr->size = (need + 4095) & ~4095; 
		header_ptr->is_used = true;

		uint64_t* canary = reinterpret_cast<uint64_t*>(reinterpret_cast<char*>(header_ptr->user_ptr()) + user_need);
		*canary = 0xDEADBEEFCAFEBABEULL;

		BinHeader* remaining_ptr = header_ptr->split(need);
		if(remaining_ptr) insert_into_bin(remaining_ptr);


		return header_ptr->user_ptr();

	}

	void deallocate(void* user_ptr)
	{
		if (!user_ptr)
		{
			std::cerr << "Attempt to deallocate a null pointer\n";
			return;
		}
		BinHeader* header_ptr = (BinHeader*)((char*)user_ptr - sizeof(BinHeader));
		header_ptr->is_used = false;
		//合并相邻的空闲块
		insert_into_bin(header_ptr);

	}

	~BinManager()
	{
		for (auto &page : os_pages_record)
		{
			#ifdef _WIN32
						VirtualFree(page.address, 0, MEM_RELEASE);
			#else
						munmap(page.address, page.size); // 只存储了地址，未记录其大小
			#endif
		}
	}

	};