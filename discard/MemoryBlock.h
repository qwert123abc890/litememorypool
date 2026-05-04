#pragma once
#include<cstdint>
class MemoryPool;
class alignas(64)  MemoryBlock
{
	friend class MemoryPool;
private:
	enum MAGIC : uint32_t
	{
		LIVING = 0xCAFEBABE, //表示未释放
		DEAD = 0xDEADBEEF  //已经释放了
	};
	MemoryBlock* next;
	char* data;
	MAGIC magic_status;
	void set_alive()
	{
		magic_status = LIVING;
	}
	void set_dead()
	{
		magic_status = DEAD;
	}
	bool is_valid()
	{
		return magic_status == LIVING;
	}
	bool is_freed()
	{
		return magic_status == DEAD;
	}
	MemoryBlock* getNext() const
	{
		return next;
	}
	void setNext(MemoryBlock* nextBlock)
	{
		next = nextBlock;
	}
public:
	MemoryBlock(size_t blockSize = 1024)
	{
		next = nullptr;
		data = new char[blockSize];
		magic_status = LIVING;
	}
	~MemoryBlock()
	{
		delete[] data;
	}
	char* getData() const
	{
		return data;
	}
	

};
