/***********************************************************
 ***********************************************************
                            platform.h, softwareҲ�����ã�ҲҪ���ĵ�
                            ���ļ���ƽ̨��ص�ʵ�ֺ�����
                            ���ṩ��IOFʹ�ã�������software��
 ***********************************************************
 ***********************************************************/

#ifndef __PRODUCT_H__
#define __PRODUCT_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <time.h>

#define GAGENT_WITH_LINUX 1 // Liunxƽ̨����ʹ����̫����
#define GAGENT_FEATURE_OTA 0

#define GAGENT_PRODUCT_X86 1
#define GAGENT_CONNECT_WITH_ETHERNET 1

#define timeval_t timeval
#define sockaddr_t sockaddr_in

extern unsigned int MS_TIMER;
#endif //__PRODUCT_H__
