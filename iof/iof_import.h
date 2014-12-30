/*******************************************************************
 *******************************************************************
 ***creat new file
 ***LIB EXPORT FUNCS FOR IOF
 * ���ļ���������LIB����ĺ������ӡ���IOF�н��йҽ�
 * ���ļ���IOF����
 * ������ʷԭ����һ����ʱ���ĺ���������
 *******************************************************************
 ******************************************************************/
#ifndef IOF_IMPORT_H
#define IOF_IMPORT_H
#include <sys/wait.h>

#include "gagent.h"


/*****************************************************************
 *****************************************************************
 ********* hardware adapter function variable
 *****************************************************************
 ****************************************************************/

/* ��λWIFIģ�� */
extern void (*DRV_GAgent_Reset)();
/* ��ȡWIFI MAC��ַ */
extern void (*DRV_GAgent_GetWiFiMacAddress)( char *pMacAddress);

/* ����/��ȡ��������(��/��)����ʧ�Դ洢�� */
extern int (*DRV_GAgent_GetConfigData)(GAgent_CONFIG_S *pConfig);
extern int (*DRV_GAgent_SaveConfigData)(GAgent_CONFIG_S *pConfig);
/* ???????? */
extern void (*DRV_GAgent_WiFiStartScan)();

/* WIFI����/�������ݵ�MCU */
extern int (*pf_Gagent_Tx_To_Mcu)(char *data, int len);
extern int (*pf_Gagent_Rx_From_Mcu)(char *data, int len);

/*  */
extern int (*DRV_Led_Red)( int onoff );
extern int (*DRV_Led_Green)( int onoff );


/*****************************************************************
 *****************************************************************
 ********* functins depend on system
 *****************************************************************
 ****************************************************************/
#define msleep(ms)      usleep((unsigned long)(ms)*1000)

/* ��ȡϵͳ����ʱ�� */
extern unsigned int (*DRV_GAgent_GetTime_S)();
extern unsigned int (*DRV_GAgent_GetTime_MS)();


#endif /* IOF_EXPORT_H */
