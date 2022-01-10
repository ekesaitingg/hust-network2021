#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"


StopWaitRdtSender::StopWaitRdtSender():expectSequenceNumberSend(0),waitingState(false)
{
	this->base = 0;
	this->nextseqnum = 0;
	this->winsize = 4;
	this->head = 0;
	this->tail = -1;     //��ʼ��������е�ͷβָ��
	memset(hasReceived, false, sizeof(hasReceived));
}


StopWaitRdtSender::~StopWaitRdtSender()
{
}



bool StopWaitRdtSender::getWaitingState() {
	return waitingState;

}




bool StopWaitRdtSender::send(const Message &message) {
	if (this->waitingState) return false;
	if (nextseqnum < (base + winsize)) {
		this->packetsending.acknum = -1; //���Ը��ֶ�
		this->packetsending.seqnum = this->nextseqnum;
		this->packetsending.checksum = 0;
		memcpy(this->packetsending.payload, message.data, sizeof(message.data));
		this->packetsending.checksum = pUtils->calculateCheckSum(this->packetsending);    //�����������ݰ�
		seqin(packetsending);																//���漴�����͵����ݰ�
		printf("��ǰ���ʹ���Ϊ[%d,%d]        ", base, (base+3)%8);
		pUtils->printPacket("���ͷ����ͱ���", this->packetsending);
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetsending.seqnum);			//�������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetsending);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		nextseqnum=(nextseqnum+1)%8;
		if (nextseqnum == (base + winsize)%8) waitingState = true;
	}
	return true;
}

void StopWaitRdtSender::receive(const Packet& ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);	//���У����Ƿ���ȷ
	packetWaitingAck = packet[head];
	//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && ((ackPkt.acknum >= this->base)||(ackPkt.acknum==0&&this->base>=6)) && ackPkt.acknum <= base + winsize - 1) 
	{
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		pns->stopTimer(SENDER, ackPkt.acknum);		//�رն�ʱ��
		hasReceived[ackPkt.acknum] = true;
		if (ackPkt.acknum == base) {
			while (hasReceived[base]) {
				pns->stopTimer(SENDER, base);
				hasReceived[base] = false;
				base = (base + 1) % 8;
				head = (head + 1) % 8;               //������ǰ����
			}
		}
		printf("������ǰ�������µķ��ʹ���Ϊ[%d,%d]\n", base, (base + 3) % 8);
	this->waitingState = false;
	}
	else if (ackPkt.acknum < base||(base==0&&ackPkt.acknum>=6)) pUtils->printPacket("���ͷ��յ�һ���ʹ���ǰ��ȷ�ϱ���",ackPkt);
	else {
		pUtils->printPacket("���ͷ��յ�һ����ȷ��ȷ�ϱ���", ackPkt);
	}
}

void StopWaitRdtSender::timeoutHandler(int seqNum) {
	printf("���ͷ���ʱ��ʱ�䵽�����·��ͳ�ʱ�����ݰ�\n");
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	packetsending = packet[(head + seqNum-base)%8];
	pUtils->printPacket("���ͷ����·��ͱ���", this->packetsending);
	pns->sendToNetworkLayer(RECEIVER, this->packetsending);
}
 