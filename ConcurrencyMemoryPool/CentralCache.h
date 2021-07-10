#pragma once
#include "Common.h"

//单例模式（保证多个线程调到同一个中心缓存）
class CentralCache
{
public:
	static CentralCache* GetInstance() // 实例
	{
		return &_sInst;
	}
	// 从中心缓存获取一定数量的对象给threadcache
	// 返回值告诉我们真实给了多少个
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	// 从SpanList或者page cache获取一个span
	Span* GetOneSpan(SpanList& list, size_t byte_size);

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
	SpanList _spanLists[NFREELISTS];

private:
	CentralCache()
	{ }

	CentralCache(const CentralCache&) = delete;
	
	//单例对象
	static CentralCache _sInst;
};