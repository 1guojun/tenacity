#define _CRT_SECURE_NO_WARNINGS 1
#include<windows.h>

 #include "ThreadCache.h"
 #include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t i, size_t size)
{
	// 获取一批对象，数量使用慢启动方式
	// SizeClass::NumMoveSize(size)是上限值
	size_t batchNum = min(SizeClass::NumMoveSize(size), _freeLists[i].MaxSize());
									//[2,512]
	// 去中心缓存获取batch_num个对象
	void* start = nullptr; //输出型参数
	void* end = nullptr;
	// 必须保证调到同一个中心缓存
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, SizeClass::RoundUp(size));
	assert(actualNum > 0);

	// >1，返回一个，剩下挂到自由链表
	// 如果一次申请多个，剩下挂起来，下次申请就不需要找中心缓存 

	// 减少锁竞争，提高效率
	if (actualNum > 1)
	{
		_freeLists[i].PushRange(NextObj(start), end, actualNum - 1);
	}

	// 慢启动增长 ，调节
	if (_freeLists[i].MaxSize() == batchNum)
	{
		_freeLists[i].SetMaxSize(_freeLists[i].MaxSize() + 1);
	}

	return start;
}
void * ThreadCache::Allocate(size_t size)
{
	size_t i = SizeClass::Index(size);
	if (!_freeLists[i].Empty())  //如果ThreadCache有直接弹出
	{
		return _freeLists[i].Pop();
	}
	else  //如果没有向CentralCache要，（策略）一次要的数量采用慢启动的方式
	{
		return FetchFromCentralCache(i, size); 
		//return nullptr;
	}
}
void ThreadCache::Deallocate(void *ptr, size_t size)
{
	size_t i = SizeClass::Index(size);
	_freeLists[i].Push(ptr);

	// List Too Long central cache 去释放
	if (_freeLists[i].Size() >= _freeLists[i].MaxSize())
	{
		ListTooLong(_freeLists[i], size);
	}
}

// 释放对象时，链表过长时，回收内存回到中心缓存
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	size_t batchNum = list.MaxSize();//把申请到的还回去
	void* start = nullptr;
	void* end = nullptr;


	list.PopRange(start, end, batchNum); 

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}

