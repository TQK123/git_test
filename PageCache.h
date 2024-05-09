#pragma once

#include"Common.h"

class PageCache
{
public:
	// ��������
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	// pc��_spanLists���ó���һ��kҳ��span
	Span* NewSpan(size_t k);

	// ͨ��ҳ��ַ�ҵ�span
	Span* MapObjectToSpan(void* obj);

	// ����cc��������span
	void ReleaseSpanToPageCache(Span* span);

private:
	SpanList _spanLists[PAGE_NUM]; // pc�еĹ�ϣ

	// ��ϣӳ�䣬��������ͨ��ҳ���ҵ���Ӧspan
	//std::unordered_map<PageID, Span*> _idSpanMap;
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap;

	ObjectPool<Span> _spanPool; // ����span�Ķ����
public:
	// ����������ר�Ÿ�һ���ӿڣ������Ҿ�ֱ�ӹ�����
	std::mutex _pageMtx; // pc�������

private: // ����������˽�У�����������ȥ��
	PageCache()
	{}

	PageCache(const PageCache& pc) = delete;
	PageCache& operator = (const PageCache& pc) = delete;

	static PageCache _sInst; // ���������
};