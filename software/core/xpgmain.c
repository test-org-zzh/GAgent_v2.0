
#include "gagent.h"
#include "lib.h"
#include "Wifi_mode.h"
#include "wifi.h"
#include "mqttxpg.h"
#include "Socket.h"
#include "lan.h"
#include "mcu.h"
#include "http.h"
#include "iof_import.h"
#include "iof_export.h"
#include "gagent_login_cloud.h"

#define GAGENT_VERSION "B4R012D0"

GAgent_CONFIG_S g_stGAgentConfigData;
XPG_GLOBALVAR g_Xpg_GlobalVar;  
u8 Service_mac[6];


/*���� ��ʱ���ͻص� main����������������ʱ��*/
/*���˵��
 * ��ʱ�����������������¼�
 * main�����е�whileѭ�����������ⲿ�¼�
 */
void GAgent_MainTimer(void)
{
    
    //DRV_MainTimer();
    return ;
}

void GAgent_Global_Status_Init(void)
{
    memset( &g_stGAgentConfigData, 0, sizeof(g_stGAgentConfigData));
    memset( &g_Xpg_GlobalVar,0,sizeof(g_Xpg_GlobalVar));

    g_Xpg_GlobalVar.connect2CloudLastTime = DRV_GAgent_GetTime_S();

    memset( &g_stMQTTBroker, 0, sizeof(g_stMQTTBroker.socketid) );
    g_stMQTTBroker.socketid = -1;
}

int GAgent_Config_Status_Init()
{
	//�Ӵ洢��ȡ�����������Ϣ
	if(DRV_GAgent_GetConfigData(&g_stGAgentConfigData) == 0)
    {
        /* �ж������Ƿ���Ч */
        if( g_stGAgentConfigData.magicNumber == GAGENT_MAGIC_NUMBER )
        {
            /* �ж�DID�Ƿ���Ч */
            DIdLen = strlen(g_stGAgentConfigData.Cloud_DId);
            if(DIdLen == GAGENT_DID_LEN)
            {
                /* ��ЧDID */
                strcpy(g_Xpg_GlobalVar.DID, g_stGAgentConfigData.Cloud_DId);
                GAgent_Printf(GAGENT_LOG, "with DID:%s,[%s] len:%d", g_Xpg_GlobalVar.DID,g_stGAgentConfigData.Cloud_DId, DIdLen);
                return 0;
            }
        }
    }

    /* ��Ҫ��ʼ��������Ϣ */
    DIdLen = 0;
    memset(&g_stGAgentConfigData, 0x0, sizeof(g_stGAgentConfigData));
    g_stGAgentConfigData.magicNumber = GAGENT_MAGIC_NUMBER;
    make_rand(g_stGAgentConfigData.wifipasscode);
    DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);

    return 1;
}

void GAgent_Init(void)
{
    int uartPacketLen=0;     
    GAgent_Printf(GAGENT_INFO,"GAgent Version: %s.", GAGENT_VERSION);
    GAgent_Printf(GAGENT_INFO,"Product Version: %s.", WIFI_HARD_VERSION);
    GAgent_Printf(GAGENT_INFO,"GAgent Compiled Time: %s, %s.\r\n",__DATE__, __TIME__);

    /*Set Gloabal varialbes*/
    GAgent_Global_Status_Init();

    if(GAgent_Config_Status_Init() == 0)
    {
        g_ConCloud_Status = CONCLOUD_REQ_M2MINFO;
        GAgent_Printf(GAGENT_LOG,"MQTT_STATUS_PROVISION");
    }
    else
    {
        g_ConCloud_Status = CONCLOUD_REQ_DID;
        GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_START");
    }
    GAgent_Printf(GAGENT_INFO, "passcode:%s,len:%d", g_stGAgentConfigData.wifipasscode,strlen(g_stGAgentConfigData.wifipasscode));

	Init_Service();

	//��ȡ����MAC ��ַ
    DRV_GAgent_GetWiFiMacAddress(Service_mac);
    sprintf(g_Xpg_GlobalVar.a_mac,"%02x%02x%02x%02x%02x%02x",Service_mac[0],Service_mac[1],Service_mac[2],
                                                 Service_mac[3],Service_mac[4],Service_mac[5]);
    g_Xpg_GlobalVar.a_mac[12]='\0';

    GAgent_Printf(GAGENT_INFO, "WiFi MAC:%s", g_Xpg_GlobalVar.a_mac);

    GAgent_GetMCUInfo( DRV_GAgent_GetTime_MS() );
    
#if (GAGENT_CONNECT_WITH_ETHERNET != 1)
    {
        DRV_GAgent_WiFiStartScan();

        //����Wifi ���ӣ� �����Ƿ��ñ������ݣ�
	    //����ʹ��AP  ����Station ģʽ
        GAgent_Wifi_Connect();        
    }
#else
    {
        GAgent_InitEthernet();
    }
#endif

	//��ʼ�������˿�
    Socket_Init();      
    signal(SIGPIPE, SIG_IGN);

    GAgent_Printf(GAGENT_INFO, "GAGENT init Over, Create MainTimer now");
	//����Wifi ����ʱ��
    GAgent_CreateTimer(GAGENT_TIMER_PERIOD,1000*(3), GAgent_MainTimer);

    return;
}

/*���������Ҫ�����Щ���ܣ��ر��ǳ�ʼ������*/
/*��Ҫ�豸g_stDefaultConfigParas*/
void GAgent_DoBusiness(void)
{
    /* �������ƶ˵����� */
    GAgent_Login_Cloud();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    GAgent_Wifi_handleEvent();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    Socket_CheckNewTCPClient();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    Socket_DoTCPServer();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    Socket_DoUDPServer();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    UART_handlePacket();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    GAgent_Ping_McuTick();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);

    return;
}

