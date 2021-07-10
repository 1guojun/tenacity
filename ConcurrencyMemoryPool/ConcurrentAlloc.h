#pragma once

#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//void* tcmalloc(size_t size)
static void* ConcurrentAlloc(size_t size)
{
	try
	{
		if (size > MAX_BYTES) //PageCache
		{
			size_t align_size = SizeClass::RoundUp(size);// �������
			size_t npage = align_size >> PAGE_SHIFT;
			Span* span = PageCache::GetInstance()->NewSpan(npage);
			span->_objsize = size;

			void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
			return ptr;
		}
		else
		{
			if (tls_threadcache == nullptr)
			{
				tls_threadcache = new ThreadCache;
			}

			return tls_threadcache->Allocate(size);
		}
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
	return nullptr;
}


static void ConcurrentFree(void* ptr)
{
	try
	{
		Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
		size_t size = span->_objsize;

		if (size > MAX_BYTES)
		{
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		}
		else
		{
			assert(tls_threadcache);
			tls_threadcache->Deallocate(ptr, size);
		}
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}