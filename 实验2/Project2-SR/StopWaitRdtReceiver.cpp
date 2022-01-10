#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtReceiver.h"


StopWaitRdtReceiver::StopWaitRdtReceiver():expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
	winsize = 4;
	base = 0;
	head = 0;
	tail = -1;
	memset(packetReceived, 0, 10*sizeof(Packet));
}


StopWaitRdtReceiver::~StopWaitRdtReceiver()
{
}

void StopWaitRdtReceiver::receive(const Packet& packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确，同时收到报文的序号等于接收方期待收到的报文序号一致
	if (checkSum == packet.checksum && packet.seqnum >= this->base && packet.seqnum <= base + winsize - 1)
	{
		pUtils->printPacket("接收方正确收到发送方的报文", packet);

		if (packetReceived[(head + packet.seqnum - base) % 10].seqnum != packet.seqnum||packet.seqnum==0) {  //已缓存的数据包不重复缓存
			memcpy(&packetReceived[(head + packet.seqnum - base) % 10], &packet, sizeof(packet));    //缓存收到的数据包
			tail = (tail + 1) % 10;
		}

		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		if (packet.seqnum == base) {
			printf("当前接收窗口为[%d,%d]\n接收方正确收到一组有序的数据包，将这些数据包递交给应用层\n",base,base+winsize-1);
			Message msg;							//取出Message，向上递交给应用层
			int i = 0;
			while (1) {
				memcpy(msg.data, packetReceived[head].payload, sizeof(packetReceived[head].payload));
				memset(&packetReceived[head], 0, sizeof(packetReceived[head]));
				pns->delivertoAppLayer(RECEIVER, msg);
				base++;
				head = (head + 1) % 10;
				if (packetReceived[head].seqnum == 0) break;

				
			}

		}
	}
	else if (packet.seqnum < expectSequenceNumberRcvd - winsize);  //不做处理
	else if (checkSum == packet.checksum && packet.seqnum >= this->base - winsize && packet.seqnum <= this->base - 1)
	{
		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方收到上一窗口内的数据包，重新发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
		}
		else {
			pUtils->printPacket("接收方丢弃超出窗口范围的数据包", packet);
		}

	}
}