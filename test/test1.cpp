#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include"MemoryPool.h"
#include <windows.h>

int main()
{
	SetConsoleOutputCP(CP_UTF8);

	MemoryPool pool(32,128);
	char* block =(char*)pool.allocate();
	char* b = (char*)pool.allocate();
	strcpy(block, "hello world");
	strcpy(b, "great");
	std::cout << "内存地址为:" << (void*)block << ",存储内容为:" << block << "\n";
	std::cout << "内存地址为:" << (void*)b << ",存储内容为:" << b << "\n";
	pool.free(block);
	pool.free(b);
	MemoryPool p1(64, 128);
	char* b1 = (char*)p1.allocate();
	char* b2 = (char*)p1.allocate();
	strcpy(b1, "achievement");
	strcpy(b2, "success");
	std::cout << "内存地址为:" << (void*)b1 << ",存储内容为:" << b1 << "\n";
	std::cout << "内存地址为:" << (void*)b2 << ",存储内容为:" << b2 << "\n";
	pool.free(b1);
	pool.free(nullptr);
	p1.free(b2 + 2);


	p1.free(b1);
	p1.free(b1);

	char* b3 = (char*)pool.allocate();
	char* b4 = (char*)pool.allocate();
	strcpy(b3, "hello world");
	strcpy(b4, "great");
	std::cout << "内存地址为:" << (void*)b3 << ",存储内容为:" << b3 << "\n";
	std::cout << "内存地址为:" << (void*)b4 << ",存储内容为:" << b4 << "\n";
	pool.free(b3);
	pool.free(b4);

}
