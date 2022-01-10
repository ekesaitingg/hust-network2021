#include "stdafx.h"
#include "Global.h"
#include "StopWaitRdtReceiver.h"


StopWaitRdtReceiver::StopWaitRdtReceiver():expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
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
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ��ͬʱ�յ����ĵ���ŵ��ڽ��շ��ڴ��յ��ı������һ��
	if (checkSum == packet.checksum && packet.seqnum >= this->base && packet.seqnum <= base + winsize - 1)
	{
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);

		if (packetReceived[(head + packet.seqnum - base) % 10].seqnum != packet.seqnum||packet.seqnum==0) {  //�ѻ�������ݰ����ظ�����
			memcpy(&packetReceived[(head + packet.seqnum - base) % 10], &packet, sizeof(packet));    //�����յ������ݰ�
			tail = (tail + 1) % 10;
		}

		lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
		if (packet.seqnum == base) {
			printf("��ǰ���մ���Ϊ[%d,%d]\n���շ���ȷ�յ�һ����������ݰ�������Щ���ݰ��ݽ���Ӧ�ò�\n",base,base+winsize-1);
			Message msg;							//ȡ��Message�����ϵݽ���Ӧ�ò�
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
	else if (packet.seqnum < expectSequenceNumberRcvd - winsize);  //��������
	else if (checkSum == packet.checksum && packet.seqnum >= this->base - winsize && packet.seqnum <= this->base - 1)
	{
		lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("���շ��յ���һ�����ڵ����ݰ������·���ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
		}
		else {
			pUtils->printPacket("���շ������������ڷ�Χ�����ݰ�", packet);
		}

	}
}