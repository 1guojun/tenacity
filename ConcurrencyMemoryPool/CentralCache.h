#pragma once
#include "Common.h"

//����ģʽ����֤����̵߳���ͬһ�����Ļ��棩
class CentralCache
{
public:
	static CentralCache* GetInstance() // ʵ��
	{
		return &_sInst;
	}
	// �����Ļ����ȡһ�������Ķ����threadcache
	// ����ֵ����������ʵ���˶��ٸ�
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	// ��SpanList����page cache��ȡһ��span
	Span* GetOneSpan(SpanList& list, size_t byte_size);

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
	SpanList _spanLists[NFREELISTS];

private:
	CentralCache()
	{ }

	CentralCache(const CentralCache&) = delete;
	
	//��������
	static CentralCache _sInst;
};