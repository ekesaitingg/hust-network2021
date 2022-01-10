#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
class StopWaitRdtSender :public RdtSender
{
private:
	int expectSequenceNumberSend;	// ��һ��������� 
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬
	Packet packetWaitingAck;		//�ѷ��Ͳ��ȴ�Ack�����ݰ�
	Packet packetsending;			//���ڷ��͵����ݰ�
	int base;                       //���ʹ��ڵĻ����
	int nextseqnum;					//��һ�����õķ������
	Packet packet[8];              //���ʹ��ڻ���
	int winsize;                    //���ڴ�С
	int head;						//�������ͷ
	int tail;						//�������β
public:

	bool getWaitingState();
	bool send(const Message &message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet &ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����
	void seqin(Packet P) {
		tail = (tail + 1) % 8;
		packet[tail] = P;
	}
	void seqout(Packet P) {
		P = packet[head];
		head = (head + 1) % 8;
	}
public:
	StopWaitRdtSender();
	virtual ~StopWaitRdtSender();
};

#endif

