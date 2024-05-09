#pragma once
#include"ThreadCache.h"
#include"PageCache.h"

// ��ʵ����tcmalloc���̵߳��������������ռ�
static void* ConcurrentAlloc(size_t size)
{
	// �������ռ䳬��256KB����ֱ�����²��ȥҪ
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size); // �Ȱ���ҳ��С����
		size_t k = alignSize >> PAGE_SHIFT; // ���������֮����Ҫ����ҳ

		PageCache::GetInstance()->_pageMtx.lock(); // ��pc�е�span���в���������
		Span* span = PageCache::GetInstance()->NewSpan(k); // ֱ����pcҪ
		span->_objSize = size; // ͳ�ƴ���256KB��ҳ
		PageCache::GetInstance()->_pageMtx.unlock(); // ����

		void* ptr = (void*)(span->_pageID << PAGE_SHIFT); // ͨ����õ���span���ṩ�ռ�
		return ptr;
	}
	else // ����ռ�С��256KB�ľ���ԭ�ȵ��߼�
	{
		/* ��ΪpTLSThreadCache��TLS�ģ�ÿ���̶߳�����һ�������໥���������Բ����ھ�
		��pTLSThreadCache�����⣬��������ֻ��Ҫ�ж�һ�ξͿ���ֱ��new���������̰߳�ȫ����*/
		if (pTLSThreadCache == nullptr)
		{
			// pTLSThreadCache = new ThreadCache; // ����malloc
			// ��ʱ���൱��ÿ���̶߳�����һ��ThreadCache����

			// �ö����ڴ��������ռ�
			static ObjectPool<ThreadCache> objPool; // ��̬�ģ�һֱ����
			objPool._poolMtx.lock();
			pTLSThreadCache = objPool.New();
			objPool._poolMtx.unlock();
		}

		//cout << std::this_thread::get_id() << " " << pTLSThreadCache << endl;

		return pTLSThreadCache->Allocate(size);
	}
}

// �̵߳�����������������տռ�
static void ConcurrentFree(void* ptr)
{			/*����ڶ�������size�����ȥ���ģ�
			����ֻ��Ϊ���ô������ܲŸ���*/
	assert(ptr);
	
	// ͨ��ptr�ҵ���Ӧ��span����Ϊǰ������ռ��
	// ʱ���Ѿ���֤��ά���Ŀռ���ҳ��ַ�Ѿ�ӳ�����
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_objSize; // ͨ��ӳ������span��ȡptr��ָ�ռ��С

	// ͨ��size�ж��ǲ��Ǵ���256KB�ģ����˾���pc
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->_pageMtx.lock(); // �ǵü�������
		PageCache::GetInstance()->ReleaseSpanToPageCache(span); // ֱ��ͨ��span�ͷſռ�
		PageCache::GetInstance()->_pageMtx.unlock(); // �ǵü�������
	}
	else // ���Ǵ���256KB�ľ���tc
	{
		pTLSThreadCache->Deallocate(ptr, size);
	}
}

