
#include "gagent.h"
#include "lib.h"
#include "Wifi_mode.h"
#include "mqttxpg.h"
#include "wifi.h"
#include "mqtt.h"
#include "http.h"

#include "iof_import.h"
#include "gagent_login_cloud.h"

/************************************************/
u8 g_Wifihotspots[1024]={0};
u16 g_WifihotspotsLen=0;
short wifiStatus=0;

void GAgent_Wifi_GetHotspots( int tcpclient,u8* buf, int bufferlen )
{
    varc Uart_varatt;
    u16 TempVarLen;
    int i;
    int totalLen;
    u8 *WifiHotspots_Ack;
    if(g_WifihotspotsLen == 0)
    {
        return;
    }
    WifiHotspots_Ack  = (u8*)malloc(1024);
    if(WifiHotspots_Ack == NULL)
    {
        return;
    }
    //varlen = flag(1b)+cmd(2b)+g_WifihotspotsLen
    TempVarLen = 1+2+g_WifihotspotsLen;    
    Uart_varatt=Tran2varc(TempVarLen);
    totalLen=4+TempVarLen+Uart_varatt.varcbty;

    WifiHotspots_Ack[0]= 0x00;
    WifiHotspots_Ack[1]= 0x00;
    WifiHotspots_Ack[2]= 0x00;
    WifiHotspots_Ack[3]= 0x03;
    //varlen
    for(i=0;i<Uart_varatt.varcbty;i++)
	{
		WifiHotspots_Ack[4+i] = Uart_varatt.var[i];
	}
    //flag
    WifiHotspots_Ack[4+Uart_varatt.varcbty]=0x00;
    //cmd
    WifiHotspots_Ack[4+Uart_varatt.varcbty+1]= 0x00;
    WifiHotspots_Ack[4+Uart_varatt.varcbty+2]= 0x0D;
    //wifiHotspots
    memcpy( &WifiHotspots_Ack[4+Uart_varatt.varcbty+3],g_Wifihotspots,g_WifihotspotsLen);    
    GAgent_Printf(GAGENT_INFO,"g_WifihotspotsLen:%d",g_WifihotspotsLen);
    send(tcpclient,WifiHotspots_Ack,totalLen,0);
    
    free(WifiHotspots_Ack);
}


void GAgent_Wifi_GetWifiVersion( int nSocket,unsigned char *pData,int datalength )
{
    u8 WifiVersion[100];
    int i;
    u16 tempVarLen=0;
    varc temp_varatt;
    tempVarLen = 9+g_busiProtocolVerLen;
    //protocolver
    WifiVersion[0]=0x00;
    WifiVersion[1]=0x00;
    WifiVersion[2]=0x00;
    WifiVersion[3]=0x03;
    temp_varatt=Tran2varc(tempVarLen);
    //varLen
    for(i=0;i<temp_varatt.varcbty;i++)
	{
		WifiVersion[4+i] = temp_varatt.var[i];
	}    
    //flag
    WifiVersion[4+temp_varatt.varcbty]=0x00;
    //cmd 
    WifiVersion[4+temp_varatt.varcbty+1]=0x00;
    WifiVersion[4+temp_varatt.varcbty+2]=0x0b;
    //genProtocolVer
    WifiVersion[4+temp_varatt.varcbty+3]=0x00;
    WifiVersion[4+temp_varatt.varcbty+4]=0x00;
    WifiVersion[4+temp_varatt.varcbty+5]=0x00;
    WifiVersion[4+temp_varatt.varcbty+6]=0x03;
    //busiProtocolVerLen
    WifiVersion[4+temp_varatt.varcbty+7]=g_busiProtocolVerLen/255;
    WifiVersion[4+temp_varatt.varcbty+8]=g_busiProtocolVerLen%255;
    //busiProtocolVerLen
    memcpy( &WifiVersion[4+temp_varatt.varcbty+9],g_busiProtocolVer,g_busiProtocolVerLen );
    GAgent_Printf(GAGENT_INFO,"In GetWifiVersion sendLen:%d",tempVarLen+temp_varatt.varcbty+4);
    send( nSocket,WifiVersion,tempVarLen+temp_varatt.varcbty+4,0 );
}


int GAgent_Wifi_GetWifiInfo( int nSocket )
{
    int wifiInfoLen=0;    
    unsigned char *wifiInfoBuf=NULL;
    int pos,i=0;
    int TempFirmwareVerLen=0;    
    TempFirmwareVerLen = g_stGAgentConfigData.FirmwareVerLen[1];      
    if( (TempFirmwareVerLen>32)||(TempFirmwareVerLen<=0) )
    {
        //当固件版本信息长度超出规定范围 hard core一个版本信息。
        TempFirmwareVerLen=4;        
        memcpy( g_stGAgentConfigData.FirmwareVer,"V1.0",4);
    }    
    //g_stGAgentConfigData
    wifiInfoLen = 4+1+1+2+8*6+2+TempFirmwareVerLen+2+32;    
    wifiInfoBuf = (u8*)malloc( wifiInfoLen );
    if( wifiInfoBuf==NULL ) return 1;
    memset( wifiInfoBuf,0,wifiInfoLen );
    //protocol 4B
    wifiInfoBuf[0]=0x00;
    wifiInfoBuf[1]=0x00;
    wifiInfoBuf[2]=0x00;
    wifiInfoBuf[3]=0x03;
    //varlen 
    wifiInfoBuf[4]= wifiInfoLen-5;
    //flag 1B
    wifiInfoBuf[5]=0x00;
    //cmd 2B
    wifiInfoBuf[6]=0x00;
    wifiInfoBuf[7]=0x14;
    //wifiHardVar 8B
    memcpy( &wifiInfoBuf[8],WIFI_HARDVER,8 );
    pos= 16;
    //wifiSoftVer 8B
    memcpy( &wifiInfoBuf[pos],WIFI_SOFTVAR,8 );
    pos +=8;
    //mcuHardVer 8B
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.hard_ver,8 );
    pos +=8;
    //mcuSoftVer 8B;
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.soft_ver,8 );
    pos +=8;
    //mcup0Ver 8B;
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.p0_ver,8 );
    pos +=8;
    //firmwareid 8B
    memcpy( &wifiInfoBuf[pos],g_stGAgentConfigData.FirmwareId,8 );
    pos +=8;
    //firmwareVerLen 2B TempFirmwareVerLen    
    wifiInfoBuf[pos] = 0;
    wifiInfoBuf[pos+1] = TempFirmwareVerLen;
    pos +=2;
    //firmwareVer FirmwareVerLen B
    memcpy( &wifiInfoBuf[pos],g_stGAgentConfigData.FirmwareVer,TempFirmwareVerLen );
    pos +=TempFirmwareVerLen;
    // productKeyLen 2b
    wifiInfoBuf[pos]=0;
    wifiInfoBuf[pos+1]=32;
    pos+=2;
    //productkey     
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.product_key,32 );
    
    send( nSocket,wifiInfoBuf,wifiInfoLen,0);    
    free( wifiInfoBuf );
    return 0;
}

void GAgent_WiFiStatusCheckTimer(void)
{
    if((wifiStatus & WIFI_MODE_STATION) == WIFI_MODE_STATION
       && (wifiStatus & WIFI_STATION_STATUS) != WIFI_STATION_STATUS)
    {
        GAgent_Printf(GAGENT_LOG,"GAgent_WiFiStatusCheckTimer , Working at AP mode(%x)\r\n", wifiStatus);
        wifiStatus = WIFI_MODE_AP;
        DRV_WiFi_SoftAPModeStart();
    }
    return;
}

void GAgent_Wifi_handleEvent(void)
{

	/* wifi状态有变化后通知MCU */
    if( wifiStatus != g_Xpg_GlobalVar.lastWifiStatus )
    {                   
        short tempWifiStatus; 
        g_Xpg_GlobalVar.lastWifiStatus = wifiStatus;
        tempWifiStatus= htons( wifiStatus );        
        GAgent_Printf(GAGENT_INFO," 3.wifiStatus:%04X [cloud:%d]",wifiStatus, g_ConCloud_Status != CONCLOUD_RUNNING);        
        GAgentV4_Write2Mcu_with_p0( 0,WIFI_STATUS2MCU,(u8*)&tempWifiStatus,2);  
    } 

    return;
}

void GAgent_InitEthernet(void)
{
    /*设置相关状态*/
    wifiStatus = wifiStatus | WIFI_STATION_STATUS | WIFI_MODE_STATION;
    return;
}



/* 如果有SSID和KEY, 以Station模式运行，并连接到指定的SSID;
 * 否则作为AP模式运行；
 */
void GAgent_Wifi_Connect()
{
	if ((g_stGAgentConfigData.flag & XPG_CFG_FLAG_CONNECTED) == XPG_CFG_FLAG_CONNECTED)//flash 里面有要加入的网络名称
	{
	    wifiStatus = WIFI_MODE_STATION;
        GAgent_Printf(GAGENT_INFO,"try to connect wifi...[%08x]", g_stGAgentConfigData.flag);
		GAgent_Printf(GAGENT_INFO,"SSID: %s",g_stGAgentConfigData.wifi_ssid);				
        GAgent_Printf(GAGENT_INFO,"KEY: %s",g_stGAgentConfigData.wifi_key);						
		DRV_WiFi_StationCustomModeStart(g_stGAgentConfigData.wifi_ssid,g_stGAgentConfigData.wifi_key);

	}
	else
	{
	    wifiStatus = WIFI_MODE_AP;
		GAgent_Printf(GAGENT_INFO,"GAgent_Wifi_Connect, Working at AP mode\r\n");
        DRV_WiFi_SoftAPModeStart();
	}

    return;
}
