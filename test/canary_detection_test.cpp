#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<cstring>
#include"ObjectPool.h"	
int main()
{
	ObjectPool<char> pool(3);
	char* c1 = pool.acquire(); std::cout << (void*)c1 << "\n";
	char* c2 = pool.acquire(); std::cout << (void*)c2 << "\n";
	char* c3 = pool.acquire(); std::cout << (void*)c3 << "\n";
	char* c4 = pool.acquire(); std::cout << (void*)c4 << "\n";
	char* c5 = pool.acquire(); std::cout << (void*)c5 << "\n";
	strcpy(c1, "12111111");
	std::cout << "The storeed content of c1:" << c1 << "\n";
	pool.release(c1);
	strcpy(c2, "1234567");
	std::cout << "The stored content of c2:" << c2 << "\n";
	pool.release(c2);
	strcpy(c3, "12345678");
	std::cout << "The stored content of c2:" << c3 << "\n";
	pool.release(c3);
}
