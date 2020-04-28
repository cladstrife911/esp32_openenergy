#ifndef _MQTT_SUB_H
#define _MQTT_SUB_H

typedef struct
{
  int HCHC;//Heures Creuses 9 char in Wh
  int HCHP;//Heures Pleines 9 char in Wh
  int PTEC;//Période Tarifaire en cours
  int IINST;//Intensité Instantanée 3 char in A
  int PAPP;//Puissance apparente 5 char in VA
}tstrMqttSub_TicTopicsValue;

extern void MqttSub_vidInit(void);
extern void MqttSub_vidTest(int IntputVal);

extern void MqttSub_vidSetTicInfo(tstrMqttSub_TicTopicsValue *pstrTicInfo);

#endif
