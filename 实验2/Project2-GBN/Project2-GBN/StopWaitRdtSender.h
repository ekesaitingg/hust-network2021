#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
class StopWaitRdtSender :public RdtSender
{
private:
	int expectSequenceNumberSend;	// 下一个发送序号 
	bool waitingState;				// 是否处于等待Ack的状态
	Packet packetWaitingAck;		//已发送并等待Ack的数据包
	Packet packetsending;			//正在发送的数据包
	int base;                       //发送窗口的基序号
	int nextseqnum;					//下一个可用的发送序号
	Packet packet[8];              //发送窗口缓存
	int winsize;                    //窗口大小
	int head;						//缓存队列头
	int tail;						//缓存队列尾
public:

	bool getWaitingState();
	bool send(const Message &message);						//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet &ackPkt);						//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkServiceSimulator调用
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

