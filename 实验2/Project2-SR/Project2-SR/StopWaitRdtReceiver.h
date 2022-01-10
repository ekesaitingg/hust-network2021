#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
class StopWaitRdtReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	// �ڴ��յ�����һ���������
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
	Packet packetReceived[8];		//������յ������ݰ�
	int base;						//���մ��ڻ����
	int head;						//�������ͷ
	int tail;                       //�������β
	int winsize;					//���մ��ڴ�С
public:
	StopWaitRdtReceiver();
	virtual ~StopWaitRdtReceiver();

public:
	
	void receive(const Packet &packet);	//���ձ��ģ�����NetworkService���� 
};

#endif
