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
		this->packetsending.acknum = -1; //忽略该字段
		this->packetsending.seqnum = this->nextseqnum;
		this->packetsending.checksum = 0;
		memcpy(this->packetsending.payload, message.data, sizeof(message.data));
		this->packetsending.checksum = pUtils->calculateCheckSum(this->packetsending);    //制作发送数据包
		seqin(packetsending);																//缓存即将发送的数据包
		printf("当前发送窗口为[%d,%d]        ", base, (base+3)%8);
		pUtils->printPacket("发送方发送报文", this->packetsending);
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetsending.seqnum);			//启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->packetsending);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
		nextseqnum=(nextseqnum+1)%8;
		if (nextseqnum == (base + winsize)%8) waitingState = true;
	}
	return true;
}

void StopWaitRdtSender::receive(const Packet& ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);	//检查校验和是否正确
	packetWaitingAck = packet[head];
	//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
	if (checkSum == ackPkt.checksum && ((ackPkt.acknum >= this->base)||(ackPkt.acknum==0&&this->base>=6)) && ackPkt.acknum <= base + winsize - 1) 
	{
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		pns->stopTimer(SENDER, ackPkt.acknum);		//关闭定时器
		hasReceived[ackPkt.acknum] = true;
		if (ackPkt.acknum == base) {
			while (hasReceived[base]) {
				pns->stopTimer(SENDER, base);
				hasReceived[base] = false;
				base = (base + 1) % 8;
				head = (head + 1) % 8;               //窗口向前滑动
			}
		}
		printf("窗口向前滑动，新的发送窗口为[%d,%d]\n", base, (base + 3) % 8);
	this->waitingState = false;
	}
	else if (ackPkt.acknum < base||(base==0&&ackPkt.acknum>=6)) pUtils->printPacket("发送方收到一发送窗口前的确认报文",ackPkt);
	else {
		pUtils->printPacket("发送方收到一不正确的确认报文", ackPkt);
	}
}

void StopWaitRdtSender::timeoutHandler(int seqNum) {
	printf("发送方定时器时间到，重新发送超时的数据包\n");
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	packetsending = packet[(head + seqNum-base)%8];
	pUtils->printPacket("发送方重新发送报文", this->packetsending);
	pns->sendToNetworkLayer(RECEIVER, this->packetsending);
}
 