#ifndef __WIFI_MODE_H_
#define __WIFI_MODE_H_

extern unsigned char g_Wifihotspots[1024];
extern unsigned short g_WifihotspotsLen;

extern void GAgent_Wifi_GetHotspots( int tcpclient,u8* buf, int bufferlen );
extern void GAgent_Wifi_GetWifiVersion( int nSocket,unsigned char *pData,int datalength );
extern void int GAgent_Wifi_GetWifiInfo( int nSocket );
extern void GAgent_Wifi_Connect(void);
extern void GAgent_Wifi_handleEvent(void); 
#endif

