#include"PageCache.h"

PageCache PageCache::_sInst; // ��������

// ��������ķ�����������һ���õݹ���
//Span* PageCache::NewSpan(size_t k)
//{
//	_pageMtx.lock();
//	Span* res = _NewSpan(k);
//	_pageMtx.unlock();
//
//	return res;
//}

// pc��_spanLists���ó���һ��kҳ��span
Span* PageCache::NewSpan(size_t k)
{
	// ����ԭ�ȵ�assert�Ѿ����ˣ���һ��
	//// ����ҳ��һ������[1, PAGE_NUM - 1]�����Χ�ڵ�
	//assert(k > 0 && k < PAGE_NUM);
	assert(k > 0);

	// ������������ҳ������128ҳʱ����Ҫ��os���룬���û�г���128ҳ�Ļ�������pc����
	if (k > PAGE_NUM - 1) 
	{
		void* ptr = SystemAlloc(k); // ֱ����os����
		//Span* span = new Span; // ��һ���µ�span�����������µĿռ�
		Span* span = _spanPool.New(); // �ö����ڴ�ؿ��ռ�
		
		span->_pageID = ((PageID)ptr >> PAGE_SHIFT); // ����ռ�Ķ�Ӧҳ��
		span->_n = k; // �����˶���ҳ
		
		// �����span�������ҳӳ�䵽��ϣ�У�������ɾ�����span��ʱ�����ҵ�����
		//_idSpanMap[span->_pageID] = span;
		_idSpanMap.set(span->_pageID, span);
		// ����Ҫ�����span��pc����pcֻ�ܹ�С��128ҳ��span

		return span;
	}

	// �� k��Ͱ����span
	if (!_spanLists[k].Empty())
	{ // ֱ�ӷ��ظ�Ͱ�еĵ�һ��span
		Span* span = _spanLists[k].PopFront();

		// ��¼�����ȥ��span�����ҳ�ź����ַ��ӳ���ϵ
		for (PageID i = 0; i < span->_n; ++i) // ע��iҪPageID���ͣ���Ȼ��64λ�º�_pageID��ӻᱨ����
		{ // nҳ�Ŀռ�ȫ��ӳ�䶼��span��ַ
			//_idSpanMap[span->_pageID + i] = span;
			_idSpanMap.set(span->_pageID + i, span);
		}

		return span;
	}

	// �� k��Ͱû��span���������Ͱ����span
	for (int i = k + 1; i < PAGE_NUM; ++i)
	{ // [k+1, PAGE_NUM - 1]��Ͱ����û��span
		if (!_spanLists[i].Empty())
		{ // i��Ͱ����span���Ը�span�����з�
			
			// ��ȡ����Ͱ�е�span�������ͽ�nSpan
			Span* nSpan = _spanLists[i].PopFront();

			// �����span�зֳ�һ��kҳ�ĺ�һ��n-kҳ��span
			
			// Span�Ŀռ�����Ҫ�½��ģ��������õ�ǰ�ڴ���еĿռ�
			//Span* kSpan = new Span;
			Span* kSpan = _spanPool.New(); // �ö����ڴ�ؿ��ռ�

			// ��һ��kҳ��span
			kSpan->_pageID = nSpan->_pageID;
			kSpan->_n = k;

			// ��һ�� n - k ҳ��span
			nSpan->_pageID += k;
			nSpan->_n -= k;

			// n - kҳ�ķŻض�Ӧ��ϣͰ��
			_spanLists[nSpan->_n].PushFront(nSpan);

			// �ٰ�n-kҳ��span��Եҳӳ��һ�£���������ϲ�
			//_idSpanMap[nSpan->_pageID] = nSpan;
			//_idSpanMap[nSpan->_pageID + nSpan->_n - 1] = nSpan;
			_idSpanMap.set(nSpan->_pageID, nSpan);
			_idSpanMap.set(nSpan->_pageID + nSpan->_n - 1, nSpan);

			// ��¼�����ȥ��kSpan�����ҳ�ź����ַ��ӳ���ϵ
			for (PageID i = 0; i < kSpan->_n; ++i) // ע��iҪPageID���ͣ���Ȼ��64λ�º�_pageID��ӻᱨ����
			{ // nҳ�Ŀռ�ȫ��ӳ�䶼��kSpan��ַ
				//_idSpanMap[kSpan->_pageID + i] = kSpan;
				_idSpanMap.set(kSpan->_pageID + i, kSpan);
			}

			return kSpan;
		}
	}

	// �� k��Ͱ�ͺ����Ͱ�ж�û��span

	// ֱ����ϵͳ����128ҳ��span
	void* ptr = SystemAlloc(PAGE_NUM - 1); // PAGE_NUMΪ129
	//cout << ptr << endl;
	// ��һ���µ�span����ά�����ռ�
	//Span* bigSpan = new Span;
	Span* bigSpan = _spanPool.New(); // �ö����ڴ�ؿ��ռ�

	/* ֻ��Ҫ�޸�_pageID��_n����,
	ϵͳ���ýӿ�����ռ��ʱ��һ���ܱ�֤����Ŀռ��Ƕ���� */
	bigSpan->_pageID = ((PageID)ptr) >> PAGE_SHIFT;
	bigSpan->_n = PAGE_NUM - 1;

	// �����span�ŵ���Ӧ��ϣͰ��
	_spanLists[PAGE_NUM - 1].PushFront(bigSpan);

	// �ݹ��ٴ�����kҳ��span����εݹ�һ�����ߢڵ��߼�
	return NewSpan(k);  // ���ô���
}

// ͨ��ҳ��ַ�ҵ�span
Span* PageCache::MapObjectToSpan(void* obj)
{ // ҳ����span
	
  // ͨ�����ַ�ҵ�ҳ��
	PageID id = (((PageID)obj) >> PAGE_SHIFT);

	// ��������һ����������������ͬѧ���Կ���ǰ��Ĳ���
	//std::unique_lock<std::mutex> lc(_pageMtx); // ����Ҫ������
	// ͨ����ϣ�ҵ�ҳ�Ŷ�Ӧspan
	//auto ret = _idSpanMap.find(id);
	auto ret = _idSpanMap.get(id);
	// ������߼���һ���ܱ�֤ͨ�����ַ�ҵ�һ��span�ģ����û�ҵ��ͳ�����
	//if (ret != _idSpanMap.end())
	if (ret != nullptr)
	{ // ����ret��һ��������
		return (Span*)ret;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

// ����cc��������span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	// ͨ��span�ж��ͷŵĿռ�ҳ���Ƿ����128ҳ���������128ҳ��ֱ�ӻ���os
	if (span->_n > PAGE_NUM - 1)
	{
		void* ptr = (void*)(span->_pageID << PAGE_SHIFT); // ��ȡ��Ҫ�ͷŵĵ�ַ
		SystemFree(ptr); // ֱ�ӵ���ϵͳ�ӿ��ͷſռ�
		//delete span; // �ͷŵ�span
		_spanPool.Delete(span); // �ö����ڴ��ɾ��span

		return;
	}

	/**************����Ķ���ԭ�ȵĴ��룬Ҳ����ҳ��С�ڵ���128ҳ��span**************/
	// ���󲻶Ϻϲ�
	while (1)
	{
		PageID leftID = span->_pageID - 1; // �õ��������ҳ
		auto ret = _idSpanMap.get(leftID); // ͨ������ҳӳ�����Ӧspan
		
		// û������span��ֹͣ�ϲ�
		if (ret == nullptr)
		{
			break;
		}

		Span* leftSpan = (Span*)ret; // ����span
		// ����span��cc�У�ֹͣ�ϲ�
		if (leftSpan->_isUse == true)
		{
			break;
		}

		// ����span�뵱ǰspan�ϲ��󳬹�128ҳ��ֹͣ�ϲ�
		if (leftSpan->_n + span->_n > PAGE_NUM - 1)
		{
			break;
		}

		// ��ǰspan������span���кϲ�
		span->_pageID = leftSpan->_pageID;
		span->_n += leftSpan->_n;

		_spanLists[leftSpan->_n].Erase(leftSpan);// ������span�����Ͱ��ɾ��
		//delete leftSpan;// ɾ��������span����
		_spanPool.Delete(leftSpan); // �ö����ڴ��ɾ��span
	}

	// ���Ҳ��Ϻϲ�
	while (1)
	{
		PageID rightID = span->_pageID + span->_n; // �ұߵ�����ҳ
		auto it = _idSpanMap.get(rightID); // ͨ������ҳ�ҵ���Ӧspanӳ���ϵ

		// û������span��ֹͣ�ϲ�
		if (it == nullptr)
		{
			break;
		}

		Span* rightSpan = (Span*)it; // �ұߵ�span
		// ����span��cc�У�ֹͣ�ϲ�
		if (rightSpan->_isUse == true)
		{
			break;
		}

		// ����span�뵱ǰspan�ϲ��󳬹�128ҳ��ֹͣ�ϲ�
		if (rightSpan->_n + span->_n > PAGE_NUM - 1)
		{
			break;
		}

		// ��ǰspan������span���кϲ�
		span->_n += rightSpan->_n; // ���ұߺϲ�ʱ����Ҫ��span->_pageID��
								   // �ұߵĻ�ֱ��ƴ��span����

		// ��Ͱ�����spanɾ��
		_spanLists[rightSpan->_n].Erase(rightSpan);
		//delete rightSpan; // ɾ���ұ�span����Ŀռ�
		_spanPool.Delete(rightSpan); // �ö����ڴ��ɾ��span
	}

	// �ϲ���ϣ�����ǰspan�ҵ���ӦͰ��
	_spanLists[span->_n].PushFront(span);
	span->_isUse = false; // ��cc���ص�pc��isUse�ĳ�false

	// ӳ�䵱ǰspan�ı�Եҳ�����������Զ����span�ϲ�
	/*_idSpanMap[span->_pageID] = span; 
	_idSpanMap[span->_pageID + span->_n - 1] = span;*/
	_idSpanMap.set(span->_pageID, span);
	_idSpanMap.set(span->_pageID + span->_n - 1, span);
}
