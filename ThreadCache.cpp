#include"ThreadCache.h"
#include"CentralCache.h"

// �߳���tc����size��С�Ŀռ�
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES); // tc�е���ֻ�����벻����256KB�Ŀռ�

	size_t alignSize = SizeClass::RoundUp(size); // size�������ֽ���
	size_t index = SizeClass::Index(size); // size��Ӧ�ڹ�ϣ���е��ĸ�Ͱ

	if (!_freeLists[index].Empty())
	{ // ���������в�Ϊ�գ�����ֱ�Ӵ����������л�ȡ�ռ�
		return _freeLists[index].Pop();
	}
	else
	{ // ��������Ϊ�գ���Ҫ�� ThreadCache �� CentralCache ����ռ�
		return FetchFromCentralCache(index, alignSize); // ����Ϊɶ����������������ὲ��
	}
}

// �����߳��д�СΪsize��obj�ռ�
void ThreadCache::Deallocate(void* obj, size_t size)
{
	assert(obj); // ���տռ䲻��Ϊ��
	assert(size <= MAX_BYTES); // ���տռ��С���ܳ���256KB

	size_t index = SizeClass::Index(size); // �ҵ�size��Ӧ����������
	_freeLists[index].Push(obj); // �ö�Ӧ����������տռ�

	// ��ǰͰ�еĿ������ڵ��ڵ��������������ʱ��黹�ռ�
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}

// ThreadCache�пռ䲻��ʱ����CentralCache����ռ�Ľӿ�
void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
#ifdef WIN32
	// ͨ��MaxSize��NumMoveSie�����Ƶ�ǰ��tc�ṩ���ٿ�alignSize��С�Ŀռ�
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
		/*MaxSize��ʾindexλ�õ���������������δ������ʱ���ܹ����������ռ��Ƕ���*/
		/*NumMoveSize��ʾtc������cc����alignSize��С�Ŀռ����������Ƕ���*/
		/*����ȡС���õ��ľ��Ǳ���Ҫ��tc�ṩ���ٿ�alignSize��С�Ŀռ�*/
		/*����˵alignSizeΪ8B��MaxSizeΪ1��NumMoveSizeΪ512���Ǿ�Ҫ��һ��8B�Ŀռ�*/
		/*Ҳ����û�����޾͸�MaxSize���������޾͸����޵�NumMoveSize*/
#else
	// ����ϵͳ�е���std
	size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
#endif // WIN32

	if (batchNum == _freeLists[index].MaxSize())
	{ //���û�е������ޣ����´����������ռ��ʱ����Զ�����һ��
		_freeLists[index].MaxSize()++; // �´ζ��һ��
		// �����������ʼ�������ڵĺ���
	}

	/*�����������ʼ���������㷨*/

	// ����Ͳ���������֮��Ľ������tc��Ҫ�Ŀռ�
	void* start = nullptr;
	void* end = nullptr;

	// ����ֵΪʵ�ʻ�ȡ���Ŀ���
	size_t actulNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);
	
	assert(actulNum >= 1); //actualNumһ���Ǵ��ڵ���1�ģ�����FetchRangeObj�ܱ�֤��

	if (actulNum == 1)
	{ // ���actulNum����1����ֱ�ӽ�start���ظ��߳�
		assert(start == end);
		return start;
	}
	else
	{ // ���actulNum����1���ͻ�Ҫ��tc��Ӧλ�ò���[ObjNext(start), end]�Ŀռ�
		_freeLists[index].PushRange(ObjNext(start), end, actulNum - 1);

		// ���̷߳���start��ָ�ռ�
		return start;
	}
}

// tc��cc�黹�ռ�
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{ 
	void* start = nullptr;
	void* end = nullptr;

	// ��ȡMaxSize��ռ�
	list.PopRange(start, end, list.MaxSize());

	// �黹�ռ�
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
