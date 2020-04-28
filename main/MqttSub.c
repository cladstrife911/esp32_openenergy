

#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "assert.h"

#include "MqttSub.h"

#include "mqtt_client.h"


/***************** LOCAL MACROS *****************/

/***************** LOCAL TYPES *****************/
typedef enum
{
  enuMqttDisconnected,
  enuMqttConnected,
}tenuMqttConnectionState;

/***************** LOCAL FUNCTIONS *****************/

static void vidInitLocalVar(void);
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);

/***************** LOCAL VAR *****************/

static const char *TAG = "[MqttSub]";

const esp_mqtt_client_config_t mqtt_cfg = {
    .uri = "mqtt://192.168.1.2",
    .event_handle = mqtt_event_handler,
    // .user_context = (void *)your_context
};

static esp_mqtt_client_handle_t LOC_strMqttClientHandle = NULL;
static char *expected_data = NULL;
static char *actual_data = NULL;
static size_t expected_size = 0;
static size_t expected_published = 0;
static size_t actual_published = 0;
static int qos_test = 0;

static tenuMqttConnectionState LOC_enuMqttConnState;

/***************** GLOBAL FUNCTIONS *****************/
void MqttSub_vidInit(void)
{
  ESP_LOGI(TAG, "## MqttSub_vidInit");
  vidInitLocalVar();

  LOC_strMqttClientHandle = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(LOC_strMqttClientHandle, ESP_EVENT_ANY_ID, mqtt_event_handler, LOC_strMqttClientHandle);

  esp_mqtt_client_start(LOC_strMqttClientHandle);
}

void MqttSub_vidTest(int IntputVal)
{
  char acTxBuffer[10];

  // esp_mqtt_client_set_uri(LOC_strMqttClientHandle, mqtt_cfg.uri);
  esp_mqtt_client_reconnect(LOC_strMqttClientHandle);

  sprintf(acTxBuffer, "%d",IntputVal);
  int msg_id = esp_mqtt_client_publish(LOC_strMqttClientHandle, "AntoineHome/TIC/IINST", acTxBuffer, expected_size, qos_test, 0);
}

void MqttSub_vidSetTicInfo(tstrMqttSub_TicTopicsValue *pstrTicInfo)
{
  char acTxBuffer[10];
  int msg_id=0;

  assert(pstrTicInfo!=NULL);

  if(LOC_enuMqttConnState == enuMqttDisconnected)
  {
    ESP_LOGW(TAG, "Reconnect to Mqtt broker.");
    esp_mqtt_client_reconnect(LOC_strMqttClientHandle);
  }

  sprintf(acTxBuffer, "%d",pstrTicInfo->HCHC);
  msg_id += esp_mqtt_client_publish(LOC_strMqttClientHandle, "AntoineHome/TIC/HCHC", acTxBuffer, expected_size, qos_test, 0);
  sprintf(acTxBuffer, "%d",pstrTicInfo->HCHP);
  msg_id += esp_mqtt_client_publish(LOC_strMqttClientHandle, "AntoineHome/TIC/HCHP", acTxBuffer, expected_size, qos_test, 0);
  sprintf(acTxBuffer, "%d",pstrTicInfo->IINST);
  msg_id += esp_mqtt_client_publish(LOC_strMqttClientHandle, "AntoineHome/TIC/IINST", acTxBuffer, expected_size, qos_test, 0);
  msg_id += esp_mqtt_client_publish(LOC_strMqttClientHandle, "emon/emonpi/power1", acTxBuffer, expected_size, qos_test, 0);
  sprintf(acTxBuffer, "%d",pstrTicInfo->PTEC);
  msg_id += esp_mqtt_client_publish(LOC_strMqttClientHandle, "AntoineHome/TIC/PTEC", acTxBuffer, expected_size, qos_test, 0);
  sprintf(acTxBuffer, "%d",pstrTicInfo->PAPP);
  msg_id += esp_mqtt_client_publish(LOC_strMqttClientHandle, "AntoineHome/TIC/PAPP", acTxBuffer, expected_size, qos_test, 0);

  if(msg_id != 0)
  {
    ESP_LOGW(TAG, "Error during publising");
  }
}


/***************** LOCAL FUNCTIONS *****************/
static void vidInitLocalVar(void)
{
  LOC_enuMqttConnState = enuMqttDisconnected;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    static int msg_id = 0;
    static int actual_len = 0;
    // your_context_t *context = event->context;
    switch (event->event_id) {
      case MQTT_EVENT_CONNECTED:
          ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
          // xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT);
          // msg_id = esp_mqtt_client_subscribe(client, CONFIG_EXAMPLE_SUBSCIBE_TOPIC, qos_test);
          // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
          LOC_enuMqttConnState = enuMqttConnected;
          break;
      case MQTT_EVENT_DISCONNECTED:
          ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
          LOC_enuMqttConnState = enuMqttDisconnected;
          break;

      case MQTT_EVENT_SUBSCRIBED:
          ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
          break;
      case MQTT_EVENT_UNSUBSCRIBED:
          ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
          break;
      case MQTT_EVENT_PUBLISHED:
          ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
          break;
      case MQTT_EVENT_DATA:
          ESP_LOGI(TAG, "MQTT_EVENT_DATA");
          // printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
          // printf("DATA=%.*s\r\n", event->data_len, event->data);
          // printf("ID=%d, total_len=%d, data_len=%d, current_data_offset=%d\n", event->msg_id, event->total_data_len, event->data_len, event->current_data_offset);
          // if (event->topic) {
          //     actual_len = event->data_len;
          //     msg_id = event->msg_id;
          // } else {
          //     actual_len += event->data_len;
          //     // check consisency with msg_id across multiple data events for single msg
          //     if (msg_id != event->msg_id) {
          //         ESP_LOGI(TAG, "Wrong msg_id in chunked message %d != %d", msg_id, event->msg_id);
          //         abort();
          //     }
          // }
          // memcpy(actual_data + event->current_data_offset, event->data, event->data_len);
          // if (actual_len == event->total_data_len) {
          //     if (0 == memcmp(actual_data, expected_data, expected_size)) {
          //         printf("OK!");
          //         memset(actual_data, 0, expected_size);
          //         actual_published ++;
          //         if (actual_published == expected_published) {
          //             printf("Correct pattern received exactly x times\n");
          //             ESP_LOGI(TAG, "Test finished correctly!");
          //         }
          //     } else {
          //         printf("FAILED!");
          //         abort();
          //     }
          // }
          break;
      case MQTT_EVENT_ERROR:
          ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
          break;
      default:
          ESP_LOGI(TAG, "Other event id:%d", event->event_id);
          break;
    }
    return ESP_OK;
}
