
#ifndef __MQTTLIB_H__
#define __MQTTLIB_H__

#include <stdint.h>

#include "mqttlib.h"
#include "MqttSTM.h"
#include "mqtt.h"

u8 g_MQTTBuffer[MQTT_SOCKET_BUFFER_LEN] = {0};
int g_MQTTStatus=0;
int DIdLen=0;
mqtt_broker_handle_t g_stMQTTBroker;

int GAgent_Mqtt_Register2Server( mqtt_broker_handle_t *Reg_broker);
int GAgent_Mqtt_Login2Server( mqtt_broker_handle_t *Reg_broker );
void GAgent_MQTT_handlePacket(void);

extern int DIdLen;
extern mqtt_broker_handle_t g_stMQTTBroker;

#endif // __LIBEMQTT_H__
