#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
struct ChannelMap {
	//struct Channel* list[]
	struct Channel** list;
	int size;// ��¼ָ��ָ��������Ԫ���ܸ���

};
//��ʼ��
struct ChannelMap* channelMapInit(int size);
//��� map
void ChannelMapClear(struct ChannelMap* map);
//���·����ڴ�ռ�
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);
