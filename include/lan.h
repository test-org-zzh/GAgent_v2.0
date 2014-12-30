#ifndef _LAN_XPG_H_
#define _LAN_XPG_H_


#define PACKET_DATA_SIZE 128

typedef struct _varc
{
	char var[2];//������ֵ
	char varcbty;//1by-4by	
} varc;

typedef struct _HeadUart
{
	char pro[4];
	char len[2];  //��ʾ�����ֽڵĳ���
	char cmd[2];	
	char socketid[4];
} HeadUart;

extern int g_passcodeEnable;
extern int g_SocketLogin[8];

extern unsigned char *g_busiProtocolVer; //ҵ���߼�Э��汾��
extern unsigned short g_busiProtocolVerLen;

int Udp_Onboarding( unsigned char* buf ,int datalength );
void DispatchTCPData( int nSocket, unsigned char *pData,int datalength);

void MCU_SendData(unsigned char *buffer,int datalen);
int  MCU_GetPacket(unsigned char *buffer, int bufferMaxLen);
varc Tran2varc(short remainLen);

void MCU_OutputLOGInfo(char *log, int len, int level);

#endif
