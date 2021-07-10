#pragma once



#include<iostream>
#include<exception>
#include<vector>
#include<time.h>
#include<map>
#include<unordered_map>
#include<assert.h>

#include<thread>
#include<mutex>
#include<algorithm>

#ifdef _WIN32
	#include<windows.h>
#else
   //...
#endif

using std::cout;
using std::endl;

static const size_t MAX_BYTES = 64 * 1024;
static const size_t NFREELISTS = 184;
static const size_t NPAGES = 129;
static const size_t PAGE_SHIFT = 12;

#ifdef _WIN32
	typedef size_t ADDRES_INT;
#else
	typedef unsigned long long ADDRES_INT;
#endif

#ifdef _WIN32
	typedef size_t PageID;
#else
	typedef unsigned long long PageID;
#endif


	
inline void*& NextObj(void* obj)
{
	return  *((void**)obj);
}

//���������ӳ��ȹ�ϵ
class SizeClass
{
public:
	// ������1%-12%���ҵ�����Ƭ�˷�
	// [1,128]					8byte����	     freelist[0,16)
	// [129,1024]				16byte����		 freelist[16,72)
	// [1025,8*1024]			128byte����	     freelist[72,128)
	// [8*1024+1,64*1024]		1024byte����     freelist[128,184)

	static inline size_t _RoundUp(size_t bytes, size_t align) //����
	{
		return (((bytes)+align - 1) & ~(align - 1));
	}
	// �����С���㣬�˷Ѵ����1%-12%����
	static inline size_t RoundUp(size_t bytes)
	{

		if (bytes <= 128){
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024){
			return  _RoundUp(bytes, 16);
		}
		else if (bytes <= 8192){
			return  _RoundUp(bytes, 128);
		}
		else if (bytes <= 65536){
			return  _RoundUp(bytes, 1024);
		}
		else
		{
			return _RoundUp(bytes, 1 << PAGE_SHIFT);
		}

		return -1;
	}
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	// ����ӳ�����һ����������Ͱ
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		// ÿ�������ж��ٸ���
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128){
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024){
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8192){
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 65536){
			return _Index(bytes - 8192, 10) + group_array[2] + group_array[1] + group_array[0];
		}

		assert(false);

		return -1;
	}

	// һ�δ����Ļ����ȡ���ٸ�
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;

		// [2, 512]��һ�������ƶ����ٸ������(������)����ֵ
		// С����һ���������޸�
		// С����һ���������޵�
		int num = MAX_BYTES / size;
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	// ����һ����ϵͳ��ȡ����ҳ
	// �������� 8byte
	// ...
	// �������� 64KB
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num*size;

		npage >>= 12; //���Ǽ�ҳ
		if (npage == 0)
			npage = 1;

		return npage;
	}
};

class FreeList
{
public:
	void PushRange(void*start, void* end, int n)
	{
		NextObj(end) = _head;
		_head = start;
		_size += n;
	}
	void PopRange(void*& start, void*& end, int n)
	{
		start = _head;
		for (int i = 0; i < n; ++i)
		{
			end = _head;
			_head = NextObj(_head);
		}

		NextObj(end) = nullptr;
		_size -= n;
	}

	//ͷ��
	void Push(void *obj)
	{
		NextObj(obj) = _head;
		_head = obj;
		_size += 1;
	}

	// ͷɾ
	void* Pop()
	{
		void* obj = _head;
		_head = NextObj(_head);
		_size -= 1;

		return obj;
	}

	bool Empty()
	{
		return _head == nullptr;
	}

	size_t MaxSize()
	{
		return _max_size;
	}

	void SetMaxSize(size_t n)
	{
		_max_size = n;
	}

	size_t Size()
	{
		return _size;
	}

private:
	void* _head = nullptr;
	size_t _max_size = 1;  //�����������ǵ�����
	size_t _size = 0;
};

////////////////////////////////////////////////////
// Span
// ��������һ����ȵĴ���ڴ�


// ������ҳΪ��λ�Ĵ���ڴ�
struct Span
{
	PageID _pageId = 0; //ҳ��
	size_t _n = 0;     // ҳ������

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _list = nullptr;  //_list==nullptr,˵����û�к��ʵ�Span,�����ڴ���С�����������������ջ������ڴ�Ҳ��������
	size_t _usecount = 0;   // ʹ�ü����� _usecount==0 ˵�����󶼻�����

	size_t _objsize = 0;   // �г����ĵ�������Ĵ�С
};

class SpanList //˫���ͷѭ��
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	void PushFront(Span* span)  //ͷ��
	{
		Insert(Begin(), span);
	}

	Span* PopFront()
	{
		Span* span = Begin();
		Erase(span);

		return span;
	}

	void Insert(Span* cur, Span* newspan)
	{
		Span* prev = cur->_prev;
		// prev newspan cur
		prev->_next = newspan;
		newspan->_prev = prev;

		newspan->_next = cur;
		cur->_prev = newspan;
	}

	void Erase(Span*cur)
	{
		assert(cur != _head);//����ɾ��ͷ���
		Span* prev = cur->_prev;
		Span*next = cur->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

	void Lock()
	{
		_mtx.lock();
	}

	void Unlock()
	{
		_mtx.unlock();
	}
private:
	Span* _head;
public:
	std::mutex _mtx;
};


inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage*(1 << PAGE_SHIFT),   // ��ϵͳ�����ڴ�ռ�
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// brk mmap��  //Linux
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap��
#endif
}