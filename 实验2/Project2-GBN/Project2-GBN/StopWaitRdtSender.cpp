#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtSender.h"


StopWaitRdtSender::StopWaitRdtSender():expectSequenceNumberSend(0),waitingState(false)
{
	this->base = 0;
	this->nextseqnum = 0;
	this->winsize = 4;
	this->head = 0;
	this->tail = -1;     //初始化缓存队列的头尾指针
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
		this->packetsending.acknum = -1; //忽略该字段
		this->packetsending.seqnum = this->nextseqnum;
		this->packetsending.checksum = 0;
		memcpy(this->packetsending.payload, message.data, sizeof(message.data));
		this->packetsending.checksum = pUtils->calculateCheckSum(this->packetsending);    //制作发送数据包
		seqin(packetsending);																//缓存即将发送的数据包
		printf("当前发送窗口为[%d,%d]", base, (base+3)%8);
		pUtils->printPacket("发送方发送报文", this->packetsending);
		if (base == nextseqnum) pns->startTimer(SENDER, Configuration::TIME_OUT, 1);			//启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->packetsending);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		nextseqnum = (nextseqnum + 1) % 8;
		if (nextseqnum == (base + winsize)%8) waitingState = true;
	}
	return true;
}

void StopWaitRdtSender::receive(const Packet& ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);	//检查校验和是否正确
	packetWaitingAck = packet[head];
	//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum && (ackPkt.acknum >= this->packetWaitingAck.seqnum||(ackPkt.acknum==0&&this->packetWaitingAck.seqnum>=6))) {
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		pns->stopTimer(SENDER, 1);		//关闭定时器
		head = (head + ackPkt.acknum-this->packetWaitingAck.seqnum+1) % 8;
		base = (ackPkt.acknum + 1)%8;
		printf("发送窗口向前滑动，新的发送窗口为[%d,%d]\n", base, (base + 3) % 8);
		if (this->base != this->nextseqnum)   pns->startTimer(SENDER, Configuration::TIME_OUT,1);
		this->waitingState = false;
	}
	else {
		pUtils->printPacket("发送方收到一不正确的确认报文", ackPkt);
	}
}

void StopWaitRdtSender::timeoutHandler(int seqNum) {
		printf("发送方定时器时间到，重新发送窗口内所有数据包\n");
		pns->stopTimer(SENDER, 1);										//首先关闭定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT, 1);			//重新启动发送方定时器
		for (int i = 0; i < (this->nextseqnum - this->base+8)%8; i++) {
			packetsending = packet[(head+i)%8];
			pUtils->printPacket("发送方重新发送报文", this->packetsending);

			pns->sendToNetworkLayer(RECEIVER, this->packetsending);
		}
}
 