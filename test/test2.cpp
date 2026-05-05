#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include"MemorySafePool.h"
#include <windows.h>

#include<iomanip>
int main()
{
    MemorySafePool pool(2, 8);

    char* p = static_cast<char*>(pool.allocate());

    std::cout << "使用前canary处内容:" << *pool.get_canary_ptr(p) << "\n";
    std::cout << "canary = 0x" << std::hex << std::uppercase << *pool.get_canary_ptr(p)  << std::dec  << "\n";
    // 越界写操作
    strcpy(p, "123456789");  // 8字节数据区放不下，超出了canary区域

    std::cout << "存储内容:" << p << "\n";  // 输出当前数据区内容

    std::cout << "数据区存储内容:" << p << "\n";  // 再次确认数据区内容

    std::cout << "数据区前8字节字符显示: ";
    std::cout.write(p, 8); std::cout << "\n";

    std::cout << "canary地址: "  << static_cast<void*>(pool.get_canary_ptr(p)) << "\n";

    unsigned char* bytes = (unsigned char*)pool.get_canary_ptr(p);
    std::cout << "canary处的内容:" ;
    for (int i = 0; i < 8; i++)
    {
        std::cout << std::hex << std::uppercase << bytes[i];
    }
    std::cout << std::dec;
    printf("\n");

    // 打印canary值
    std::cout << "free前canary处内容: " << std::hex << std::uppercase << *pool.get_canary_ptr(p) << "\n";  // 使用*来打印canary的内容
    pool.free(p);  // 释放

    std::cout << "free后数据区的内容: " << p << "\n";  // 应该看到数据区已被清除为0xDD
    std::cout << "free后canary处内容: " << std::hex << std::uppercase << *pool.get_canary_ptr(p) << "\n";  // 确保canary仍然存在

    return 0;
}
