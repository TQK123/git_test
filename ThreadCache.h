#pragma once

#include"Common.h"

class ThreadCache
{
public:
	// �߳�����size��С�Ŀռ�
	void* Allocate(size_t size);

	// �����߳��д�СΪsize��obj�ռ�
	void Deallocate(void* obj, size_t size);

	// ThreadCache�пռ䲻��ʱ����CentralCache����ռ�Ľӿ�
	void* FetchFromCentralCache(size_t index, size_t alignSize);

	// tc��cc�黹�ռ�ListͰ�еĿռ�
	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList _freeLists[FREE_LIST_NUM]; // ��ϣ��ÿ��Ͱ��ʾһ����������
};

// TLS��ȫ�ֶ����ָ�룬����ÿ���̶߳�����һ��������ȫ�ֶ���
// static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr; // ==> _declspec(thread)��Windows���еģ��������б�������֧��
//ע��Ҫ����static�ģ���Ȼ�����.cpp�ļ��������ļ���ʱ��ᷢ�����Ӵ���

static thread_local ThreadCache* pTLSThreadCache = nullptr; // thread_local��C++11�ṩ�ģ��ܿ�ƽ̨