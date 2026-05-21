#include <iostream>
#include <vector>
#include "BinHeader.h"
#include "BinManager.h"
int main() {
    BinManager my_malloc;

    std::cout << "========================================\n";
    std::cout << "【通道一验证】：OS批发与64号超级肉池动态切肉\n";
    std::cout << "========================================\n";

    // 第一次申请 32 字节。
    // 此时 0~63 桶为空，64 号桶为空。
    // 触发保底：向 OS 批发 4KB，挂入 64 号桶，并在原地切走 32 字节。
    void* ptr1 = my_malloc.allocate(32);
    std::cout << "成功分配 ptr1 (32字节), 地址: " << ptr1 << "\n";

    // 第二次申请 64 字节。
    // 此时 0~63 桶依旧为空，但 64 号超级肉池里躺着刚才剩下的约 4000 字节！
    // 触发 64 号肉池 Best-Fit 检索，命中这块大肉，再次切走 64 字节，不触发系统调用。
    void* ptr2 = my_malloc.allocate(64);
    std::cout << "成功分配 ptr2 (64字节), 地址: " << ptr2 << "\n";


    std::cout << "\n========================================\n";
    std::cout << "【通道二验证】：0~63号小桶的极速精准复用\n";
    std::cout << "========================================\n";

    // 调用你写好的 deallocate，把刚才拿到的内存还回去。
    // 此时大管家会根据它们的实际大小，通过公式计算下标：
    // ptr1 (32字节) 自动头插归队到 -> index 0 桶
    // ptr2 (64字节) 自动头插归队到 -> index 2 桶
    std::cout << "释放 ptr1 和 ptr2...\n";
    my_malloc.deallocate(ptr1);
    my_malloc.deallocate(ptr2);

    // 再次申请 32 字节！
    // 此时大管家查表，发现 0 号等差桶（index 0）里刚好躺着刚才 ptr1 释放还回来的肉！
    // 触发 for 循环第一次直接命中！零切肉开销、零大桶遍历开销，实现 O(1) 极速精准复用。
    void* retry_ptr1 = my_malloc.allocate(32);
    std::cout << "再次分配 32 字节, 命中 index 0 小桶快速通道！新地址: " << retry_ptr1 << "\n";

    // 验证地址是否完美复用
    if (retry_ptr1 == ptr1) {
        std::cout << "🎉 完美！新地址与旧地址完全一致，小桶精准复用通道彻底打通！\n";
    }

    // 再次申请 64 字节！
    // 同理，直接命中 index 2 小桶快速通道。
    void* retry_ptr2 = my_malloc.allocate(64);
    std::cout << "再次分配 64 字节, 命中 index 2 小桶快速通道！新地址: " << retry_ptr2 << "\n";

    // 最后的清理归还
    my_malloc.deallocate(retry_ptr1);
    my_malloc.deallocate(retry_ptr2);

    std::cout << "\n========================================\n";
    std::cout << "测试全部圆满结束！大管家即将析构，账本全场安全收网归还 OS...\n";
    std::cout << "========================================\n";

    return 0;
}
