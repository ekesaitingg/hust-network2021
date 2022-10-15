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
}


StopWaitRdtSender::~StopWaitRdtSender()
{
}



bool StopWaitRdtSender::getWaitingState() {
	return waitingState;

}




bool StopWaitRdtSender::send(const Message &message) {
	if (this->waitingState) return false;
	if (nextseqnum < base + winsize) {
		this->packetsending.acknum = -1; //���Ը��ֶ�
		this->packetsending.seqnum = this->nextseqnum;
		this->packetsending.checksum = 0;
		memcpy(this->packetsending.payload, message.data, sizeof(message.data));
		this->packetsending.checksum = pUtils->calculateCheckSum(this->packetsending);    //�����������ݰ�
		seqin(packetsending);																//���漴�����͵����ݰ�
		printf("��ǰ���ʹ���Ϊ[%d,%d]", base, (base+3)%8);
		pUtils->printPacket("���ͷ����ͱ���", this->packetsending);
		if (base == nextseqnum) pns->startTimer(SENDER, Configuration::TIME_OUT, 1);			//�������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetsending);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		nextseqnum = (nextseqnum + 1) % 8;
		if (nextseqnum == (base + winsize)%8) waitingState = true;
	}
	return true;
}

void StopWaitRdtSender::receive(const Packet& ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);	//���У����Ƿ���ȷ
	packetWaitingAck = packet[head];
	//���У�����ȷ������ȷ�����=���ͷ��ѷ��Ͳ��ȴ�ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && (ackPkt.acknum >= this->packetWaitingAck.seqnum || (ackPkt.acknum == 0 && this->packetWaitingAck.seqnum >= 6))) {
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		ackCounter[ackPkt.acknum] = 0;
		pns->stopTimer(SENDER, 1);		//�رն�ʱ��
		head = (head + ackPkt.acknum - this->packetWaitingAck.seqnum + 1) % 8;
		base = (ackPkt.acknum + 1) % 8;
		if (this->base != this->nextseqnum)   pns->startTimer(SENDER, Configuration::TIME_OUT, 1);
		this->waitingState = false;
	}
	else {
		pUtils->printPacket("���ͷ��յ�һ����ȷ��ȷ�ϱ���", ackPkt);
		if (ackPkt.acknum >= 0) ackCounter[ackPkt.acknum]++;
		if (base!=nextseqnum&&ackPkt.acknum >= 0 && ackCounter[ackPkt.acknum] == 3) {
			printf("���ͷ��յ�3���ظ�ack,�� acknum = %d ���ط���һ�����ݰ�\n",ackPkt.acknum);
			pns->stopTimer(SENDER, 1);										//���ȹرն�ʱ��
			pns->startTimer(SENDER, Configuration::TIME_OUT, 1);
			packetsending = packet[(ackPkt.acknum+1)%8];
				pUtils->printPacket("���ͷ����·��ͱ���", this->packetsending);
				pns->sendToNetworkLayer(RECEIVER, this->packetsending);
		}
	}
}

void StopWaitRdtSender::timeoutHandler(int seqNum) {
		printf("���ͷ���ʱ��ʱ�䵽�����·��ʹ������������ݰ�\n");
		pns->stopTimer(SENDER, 1);										//���ȹرն�ʱ��
		pns->startTimer(SENDER, Configuration::TIME_OUT, 1);			//�����������ͷ���ʱ��

			packetsending = packet[head];
			pUtils->printPacket("���ͷ����·��ͱ���", this->packetsending);
			pns->sendToNetworkLayer(RECEIVER, this->packetsending);
		}
 