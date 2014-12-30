#ifdef  __cplusplus
extern "C"{
#endif

#include "gagent.h"
#include "lib.h"
#include "mqttxpg.h"
#include "MqttSTM.h"
#include "mqttxpg.h"
#include "mqttlib.h"
#include "Wifi_mode.h"
#include "lan.h"
#include "lib.h"
#include "Socket.h"
#include "wifi.h"
#include "http.h"    
#include "iof_import.h"
#include "gagent_login_cloud.h"

/****************************************************
*****************************************************
***************LOCAL MACO **************************
*****************************************************
*****************************************************/
#define MQTT_SOCKET_BUFFER_LEN 2048

/****************************************************
*****************************************************
***************LOCAL VARIABLES **********************
*****************************************************
*****************************************************/
static uint16_t g_Msgsub_id=0;
static int g_pingpacketack = 0;
static char Clientid[24];

/******************************************************************************
 ******************************************************************************
 ******************************************************************************
 *************                              LOCAL FUNCTIONS                       *******************
 ******************************************************************************
 ******************************************************************************
 *****************************************************************************/
static int GetAskOtaPayload()
{
    char HAsk[6]={0x00,0x00,0x00,0x03,0x02,0x05	};
	int Len,i;
    char *AskOtaPay;

	AskOtaPay = (char*)malloc(100);
    if (AskOtaPay == NULL)
    {
        free( AskOtaPay );
        return 0;
    }
	memset(AskOtaPay,0,100);
	memcpy(AskOtaPay,HAsk,6);
    AskOtaPay[6] = 0x00;
    AskOtaPay[7] = g_productKeyLen;	
    memcpy( AskOtaPay+8,g_Xpg_GlobalVar.Xpg_Mcu.product_key,32 );    
    Len = 8+g_productKeyLen;
	memcpy(AskOtaPay+Len,g_stGAgentConfigData.FirmwareId,8);
	Len=Len+8;
	//genProtocolVer[4]
	AskOtaPay[Len]	=0x00;
	AskOtaPay[Len+1]=0x00;
	AskOtaPay[Len+2]=0x00;
	AskOtaPay[Len+3]=0x03;
	Len+=4;
    // g_busiProtocolVerLen
    AskOtaPay[Len] = g_busiProtocolVerLen/255;
    AskOtaPay[Len+1] = g_busiProtocolVerLen%255;	
	Len+=2;
    //g_busiProtocolVer
	memcpy(AskOtaPay+Len,g_busiProtocolVer,g_busiProtocolVerLen);
	Len+=g_busiProtocolVerLen;    
    GAgent_MQTT_PubMsg( &g_stMQTTBroker,"cli2ser_req",AskOtaPay,Len,0);    
    free( AskOtaPay );
	return Len;
}

static char *GetRegClientid()
{
	int i;
	Clientid[0]= 'a';
	Clientid[1]= 'n';
	Clientid[2]= 'o';
	memset(Clientid+3,0,20);
	for( i=3;i<23;i++ )
	{
		Clientid[i] = 65+rand()%(90-65);
		
	}
	Clientid[23] = '\0';
    GAgent_Printf(GAGENT_INFO,"Clientid:%s\r\n",Clientid);
    
	return Clientid;
}

/**************************************************
*
*
*		return :0 
*
***************************************************/
static int mqtt_init_subTopic( mqtt_broker_handle_t* broker ,char *Sub_TopicBuf,int Mqtt_Subflag)
{    
    switch(Mqtt_Subflag)
    {
        case 1:            
            //4.6
            memcpy(Sub_TopicBuf,"ser2cli_noti/",strlen("ser2cli_noti/"));            
            memcpy(Sub_TopicBuf+strlen("ser2cli_noti/"),g_Xpg_GlobalVar.Xpg_Mcu.product_key,32);
            Sub_TopicBuf[strlen("ser2cli_noti/")+g_productKeyLen] = '\0';
            break;
        case 2:
            // 4.7 4.9		
            memcpy(Sub_TopicBuf,"ser2cli_res/",strlen("ser2cli_res/"));
            memcpy(Sub_TopicBuf+strlen("ser2cli_res/"),g_Xpg_GlobalVar.DID,strlen(g_Xpg_GlobalVar.DID));
            Sub_TopicBuf[strlen("ser2cli_res/")+strlen(g_Xpg_GlobalVar.DID)] = '\0';	
            break;
        case 3:
            // 4.13			
            memcpy(Sub_TopicBuf,"app2dev/",strlen("app2dev/"));
            memcpy(Sub_TopicBuf+strlen("app2dev/"),g_Xpg_GlobalVar.DID,strlen(g_Xpg_GlobalVar.DID));
            Sub_TopicBuf[strlen("app2dev/")+strlen(g_Xpg_GlobalVar.DID)] = '/';
            Sub_TopicBuf[strlen("app2dev/")+strlen(g_Xpg_GlobalVar.DID)+1] = '#';
            Sub_TopicBuf[strlen("app2dev/")+strlen(g_Xpg_GlobalVar.DID)+2] = '\0';        
            break;
        default:
            break;
    }

	return 0;
}

static int Mqtt_IntoRunning(void)
{
    g_MQTTStatus=MQTT_STATUS_RUNNING;
    GAgent_Printf(GAGENT_INFO, "MQTT_STATUS_RUNNING");
    #if (GAGENT_FEATURE_OTA  == 1)    
    {
	    GetAskOtaPayload();
    }
    #endif
    return 0;
}

/***********************************************************
*
*   ���� :  ת��p0 ����
*
*************************************************************/
static int MQTT_DoCloudMCUCmd(u8 clientid[32], u8 did[32], u8 *pHiP0Data, int P0DataLen)
{
    int varlen;
    int datalen;
    u8 *pP0Data;
    int pP0DataLen;
    int i;

    /*���ݱ����еı��ĳ���ȷ�������Ƿ���һ����Ч�ı���*/
    varlen = mqtt_num_rem_len_bytes(pHiP0Data+4);
    /*����ط�+3����ΪMQTT������ʵ�ְ� UDP flag�㵽messagelen���棬����Ϊ�˸�mqtt�Ᵽ��һ�����Լ�3*/
    datalen = mqtt_parse_rem_len(pHiP0Data+3); 
    
    pP0DataLen = datalen-3;//��Ϊ flag(1B)+cmd(2B)=3B
    
    //����payload��ʼ�ĵط�
    pP0Data = &pHiP0Data[7+varlen]; 
    i = GAgentV4_Write2Mcu_with_p0( 0, MCU_CTRL_CMD,pP0Data,pP0DataLen );

    GAgent_Printf(GAGENT_INFO, "MCU Do CLOUD CMD return:%d", i);
    
    return 0;
}

/*******************************************************
*
*���سɹ�����Topic�ĸ���
*
********************************************************/
static int Mqtt_SubLoginTopic( mqtt_broker_handle_t *LoginBroker)
{
    char Sub_TopicBuf[100];
    char Topic[100];
    
    memset(Sub_TopicBuf,0,100);       
    memset(Topic,0,100);
    
    switch(g_MQTTStatus)
    {
        case MQTT_STATUS_LOGIN:
            mqtt_init_subTopic(LoginBroker,Sub_TopicBuf,1);
            if(mqtt_subscribe( LoginBroker,Sub_TopicBuf,&g_Msgsub_id )==1)
            {   
                sprintf(Topic,"LOGIN sub topic is:%s",Sub_TopicBuf);
                GAgent_Printf(GAGENT_INFO,Topic);
                g_MQTTStatus=MQTT_STATUS_LOGINTOPIC1;
                GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_LOGINTOPIC1");
            }
            break;            
        case MQTT_STATUS_LOGINTOPIC1:
            mqtt_init_subTopic(LoginBroker,Sub_TopicBuf,2);
            if(mqtt_subscribe( LoginBroker,Sub_TopicBuf,&g_Msgsub_id )==1)
            {    
                sprintf(Topic,"LOGIN T1 sub topic is:%s",Sub_TopicBuf);                                
                GAgent_Printf(GAGENT_INFO,Topic);
                g_MQTTStatus=MQTT_STATUS_LOGINTOPIC2;
                GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_LOGINTOPIC2");
            }                
            break;
        case MQTT_STATUS_LOGINTOPIC2:
            mqtt_init_subTopic(LoginBroker,Sub_TopicBuf,3);
            if(mqtt_subscribe( LoginBroker,Sub_TopicBuf,&g_Msgsub_id )==1)
            {
                sprintf(Topic,"LOGIN T2 sub topic is:%s",Sub_TopicBuf);
                GAgent_Printf(GAGENT_INFO,Topic);             
                g_MQTTStatus=MQTT_STATUS_LOGINTOPIC3;
                GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_LOGINTOPIC3");
            }                
            break;
        case MQTT_STATUS_LOGINTOPIC3:
            Mqtt_IntoRunning();                
            break;           
        default:
            break;                
    }

    return 0;
}

/***********************************************************
*
*   return :    0 success ,1 error
*
*************************************************************/
static int Mqtt_SendConnetPacket( mqtt_broker_handle_t *pstBroketHandle, char *pDID )
{       
    char TmpPasscode[12];    
    int ret;
    
    if( pDID==NULL)//������¼
    {
        mqtt_init(pstBroketHandle, GetRegClientid()); 
        GAgent_Printf(GAGENT_INFO,"MQTT register with clientid:%s", Clientid);
    }
    else //�û���¼      
    {
        memcpy(TmpPasscode, g_stGAgentConfigData.wifipasscode,10);
        TmpPasscode[10]='\0';                  
        mqtt_init(pstBroketHandle,pDID);
        GAgent_Printf(GAGENT_INFO,"MQTT login with passcode:%s, DID:%s", g_stGAgentConfigData.wifipasscode, pDID);
        mqtt_init_auth(pstBroketHandle,pDID,TmpPasscode);
    }
    ret = GAgent_MQTT_initSocket(pstBroketHandle,0);
    if(ret == 0)
    {
        ret = mqtt_connect(pstBroketHandle);
        if (ret != 0)
        {
            GAgent_Printf(GAGENT_WARNING,"mqtt connect is failed with:%d", ret); 
            return 1;
        }
        return 0;                    
    }
    GAgent_Printf(GAGENT_LOG, "Mqtt_SendConnetPacket failed, GAgent_MQTT_initSocket ret:%d", ret); 
    return 1;
}

/***********************************************************
*
*   ���� : MQTT ����������, ��5������û�յ�ack,
*                  �����µ�¼�ƶ˷�����
*
*************************************************************/
static void MQTT_HeartbeatTime()
{
    //pingpacketack=0 ��ʾ�յ��ƶ��������ķ���
    if(g_pingpacketack==0)
    {
        g_pingpacketack=1;
    }
    else
    {
        g_pingpacketack++;
    }
    mqtt_ping(&g_stMQTTBroker); 
    /*
         ������5����������û�յ��ƶ˵�ack,��Ϊ���ƶ˶Ͽ����ӡ�
        ��ģ�����µ�¼��
        */
    if( g_pingpacketack>=5 )
    {
        g_pingpacketack=1;
        g_MQTTStatus= MQTT_STATUS_LOGIN;
        
        close( g_stMQTTBroker.socketid );
        g_stMQTTBroker.socketid = -1;
        GAgent_Mqtt_Login2Server( &g_stMQTTBroker);
    }
}

static int Mqtt_DoLogin( mqtt_broker_handle_t *LOG_Sendbroker,u8* packet,int packetLen )
{
    if(packet[3] != 0x00)
    {        
        GAgent_Printf(GAGENT_WARNING,"Mqtt_DoLogin CONNACK failed![%d]. restart register now ", packet[3]);
        /*����ע��*/
        g_ConCloud_Status = CONCLOUD_REQ_DID;
        
        GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_START");
        return 0;
    }
    Mqtt_SubLoginTopic( LOG_Sendbroker );	
    GAgent_CreateTimer(GAGENT_TIMER_PERIOD, 1000*(MQTT_HEARTBEAT),MQTT_HeartbeatTime);
    return 0;
}

/***********************************************************
*
*   ���� : �����ַ��������·�������
*
*************************************************************/
int Mqtt_DispatchPublishPacket( mqtt_broker_handle_t *pstMQTTBroker, u8 *packetBuffer,int packetLen )
{
    u8 topic[128];
    int topiclen;
    u8 *pHiP0Data;
    int HiP0DataLen;
    int i;
    u8  clientid[32];
    int clientidlen = 0;
    u8 *pTemp;
    u8  DIDBuffer[32];
    u8  Cloud_FirmwareId[8]; 
    u16 cmd;
    u16 *pcmd=NULL;
    u8 MQTT_ReqBinBuf[14]=
    {
        0x00,0x00 ,0x00 ,0x03 ,0x02 ,0x07 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x8c
    };            
    topiclen = mqtt_parse_pub_topic(packetBuffer, topic);
    HiP0DataLen = packetLen - topiclen;
    topic[topiclen] = '\0';
    pHiP0Data = malloc(HiP0DataLen);
    if (pHiP0Data == NULL) 
    {
        free(pHiP0Data);
        return -1;
    }

    memset(pHiP0Data, 0x0, HiP0DataLen);
    HiP0DataLen = mqtt_parse_publish_msg(packetBuffer, pHiP0Data); 
    pcmd = (u16*)&pHiP0Data[4];
    cmd = ntohs( *pcmd );
    
    if(strncmp(topic,"app2dev/",strlen("app2dev/"))==0)
    {
        pTemp = &topic[strlen("app2dev/")];
        i = 0;
        while (*pTemp != '/')
        {
            DIDBuffer[i] = *pTemp;
            i++;
            pTemp++;
        }
        DIDBuffer[i] = '\0';

        if (DIdLen == 0)
        {
            memset(g_Xpg_GlobalVar.DID,0,GAGENT_DID_LEN_MAX);
            memcpy(g_Xpg_GlobalVar.DID,DIDBuffer,i);
            memset(g_stGAgentConfigData.Cloud_DId, 0x0, sizeof(g_stGAgentConfigData.Cloud_DId));
            memcpy(g_stGAgentConfigData.Cloud_DId, DIDBuffer,i);
            g_stGAgentConfigData.Cloud_DId[i] = '\0';
            DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);
            
            DIdLen = i;
        }
        pTemp ++; /*����\/*/
        i=0;
        while (*pTemp != '\0')
        {
            clientid[i] = *pTemp;
            i++;
            pTemp++;
        }
        clientid[i]= '\0';          
        strcpy( g_Xpg_GlobalVar.phoneClientId,clientid );
        MQTT_DoCloudMCUCmd( clientid, DIDBuffer, pHiP0Data, HiP0DataLen);
    }
    //�������¹̼���Ӧ
    else if(strncmp(topic,"ser2cli_res/",strlen("ser2cli_res/"))==0)
    {
        //pHiP0Data��Ϣ���ָ��
        //HiP0DataLen��Ϣ��ĳ��� packetBuffer
        
        #if (GAGENT_FEATURE_OTA == 1)
        {      
            GAgent_Printf(GAGENT_INFO," MQTT Respond Topic1: %s\r\n",topic);            
            memcpy( Cloud_FirmwareId,pHiP0Data+6,8);            
            for( i=0;i<8;i++)
            {
                if( Cloud_FirmwareId[i]!=0 ) break;
            }
            
            if(i==8)
            {
                free( pHiP0Data );
                GAgent_Printf(GAGENT_WARNING,"cloud have no Firmware!");
                return 0;
            }
            
            if( (cmd==0x0206))
            {
                if(memcmp(g_stGAgentConfigData.FirmwareId,Cloud_FirmwareId,8)!=0)
                {
                    memcpy(MQTT_ReqBinBuf+6,Cloud_FirmwareId,8);                                 
                    GAgent_MQTT_PubMsg( &g_stMQTTBroker,"cli2ser_req",MQTT_ReqBinBuf,14,0);                    
                }
            }            
           free( pHiP0Data );
        }
        #endif
        return -3;
    } 
    else if(strncmp(topic,"ser2cli_noti/",strlen("ser2cli_noti/"))==0)
    {
        ;
    }
    else
    {
        ;
    }    
    free( pHiP0Data );
    return 0;
}

/******************************************************************************
 ******************************************************************************
 ******************************************************************************
 ************                              GLOBAL FUNCTIONS                       *******************
 ******************************************************************************
 ******************************************************************************
 *****************************************************************************/

/***********************************************************
*
*   ���� : ���ƶ�ע�ᣬ����passcode/mac addr/product key
*
*************************************************************/
int GAgent_Mqtt_Register2Server( mqtt_broker_handle_t *Reg_broker )
{
    int ret;               
    ret = Http_InitSocket( 1 );      
    if(ret!=0) return 1;    
    ret = Http_POST( HTTP_SERVER,g_stGAgentConfigData.wifipasscode,g_Xpg_GlobalVar.a_mac,g_Xpg_GlobalVar.Xpg_Mcu.product_key );        
    if( ret!=0 )
    {
        GAgent_Printf(GAGENT_INFO,"5 GAgent_Mqtt_Register2Server ");
        return 1;
    }    
    GAgent_Printf(GAGENT_INFO,"6 GAgent_Mqtt_Register2Server ");
    return 0;
}

/***********************************************************
*
*   ���� : ��¼�ƶ˷�����, ����TCP ������
*
*************************************************************/
int GAgent_Mqtt_Login2Server( mqtt_broker_handle_t *Reg_broker )
{
    GAgent_Printf(GAGENT_INFO, "login2ser [%s]%d, len:%d", g_Xpg_GlobalVar.DID, strlen(g_Xpg_GlobalVar.DID), DIdLen);
    if( Mqtt_SendConnetPacket(Reg_broker,g_Xpg_GlobalVar.DID) == 0)
    {    
        GAgent_Printf(GAGENT_INFO," Mqtt_SendConnetPacket OK!");
        return 0;
    }   
    GAgent_Printf(GAGENT_INFO," Mqtt_SendConnetPacket NO!");    
    return 1;       
}

void GAgent_MQTT_handlePacket()
{
    int packetLen = 0;
    u8 packettype=0;    
    char tmp[100];
    int Recmsg_id;
    int ret;
    
    if( g_stMQTTBroker.socketid == -1)
    {
        return;
    }

    packetLen = GAgent_MQTT_readPacket(g_MQTTBuffer,MQTT_SOCKET_BUFFER_LEN);	         
	if(packetLen>0)  				
	{
	    packettype = MQTTParseMessageType(g_MQTTBuffer);
        if ( packettype != MQTT_MSG_PINGRESP)
        {
            GAgent_Printf(GAGENT_INFO,"MQTT PacketType:%08x[g_MQTTStatus:%08x]", 
            packettype, g_MQTTStatus);
        }
        
        switch(packettype)
        {
            case MQTT_MSG_PINGRESP:
                g_pingpacketack=0;
                break;
                
            case MQTT_MSG_CONNACK:                          
                if( g_MQTTStatus==MQTT_STATUS_LOGIN )
                {                        
                    Mqtt_DoLogin( &g_stMQTTBroker,g_MQTTBuffer,packetLen );
                }                             
                break;           
            case MQTT_MSG_SUBACK:
            {
                Recmsg_id = mqtt_parse_msg_id(g_MQTTBuffer);
             
                if(g_Msgsub_id == Recmsg_id)
                {
                    Mqtt_SubLoginTopic(&g_stMQTTBroker);
                }                                  
                break;
            }
            case MQTT_MSG_PUBLISH:
            {                    
                if(g_MQTTStatus==MQTT_STATUS_RUNNING)
                {
                    ret = Mqtt_DispatchPublishPacket(&g_stMQTTBroker,g_MQTTBuffer,packetLen);
                    if (ret != 0)
                    {
                        GAgent_Printf(GAGENT_WARNING,"Mqtt_DispatchPublishPacket return:%d ",ret);
                    }
                    break;                               
                }
                else
                {
                    GAgent_Printf(GAGENT_WARNING,"Receive MQTT_MSG_PUBLISH packet but MQTTStatus is:%d ",g_MQTTStatus);
                }
                break ;
            }
            default:
                GAgent_Printf(GAGENT_INFO, "MQTTStatus is:%d , Invalid packet type:%08x",g_MQTTStatus, packettype);
                break;
		}
    }

    #if (GAGENT_FEATURE_OTA == 1)
    {
        if( packetLen==-3&&(g_MQTTStatus==MQTT_STATUS_RUNNING) )
        {
            int topiclen;
            int VARLEN;
            int mqttHeadLen; 

            u8 Mtopic[100]={0};
            
            GAgent_Printf(GAGENT_INFO, "OTA MQTTStatus is:%d ",g_MQTTStatus);     
            
            VARLEN = mqtt_num_rem_len_bytes(g_MQTTBuffer);
            topiclen = mqtt_parse_pub_topic(g_MQTTBuffer, Mtopic);                                 
            
            mqttHeadLen = 1+VARLEN+2+topiclen;
            packettype = MQTTParseMessageType(g_MQTTBuffer);
                        
            GAgent_Printf(GAGENT_INFO,"topic:%s\r\n",Mtopic);            
            if( packettype == MQTT_MSG_PUBLISH )
            {            
                if(strncmp(Mtopic,"ser2cli_res/",strlen("ser2cli_res/"))==0)
                {                
                    //SOCKET_RECBUFFER_LEN Ϊ���յ������ݳ���
                    DRV_OTAPacketHandle(g_MQTTBuffer+mqttHeadLen, MQTT_SOCKET_BUFFER_LEN-mqttHeadLen, g_stMQTTBroker.socketid);
                }
            }               
        }
    
    }
    #endif
    return;
}

#ifdef  __cplusplus
}
#endif /* end of __cplusplus */

