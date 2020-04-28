/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "TicHandler.h"
#include "EmonClient.h"
#include "MqttSub.h"

#include "MainApplication_cfg.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/****************** local macros and constants *****************/
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/*1s timeout*/
#define MICROSEC_TO_SEC (1000000)
#define u64TIMER_TIMEOUT_SECONDS (1*MICROSEC_TO_SEC)

/****************** local type *****************/
typedef enum
{
  enuDisconnected,
  enuConnected,
}tenuConnectionState;

/****************** local functions declarations *****************/
static void periodic_timer_callback(void* arg);
static void vidInitLocalVar(void);

/****************** local variables *****************/
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "[Main]";

static int s_retry_num = 0;

static tenuConnectionState LOC_enuConnState;

static const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };
static esp_timer_handle_t periodic_timer;

/****************** local functions definitions *****************/

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        LOC_enuConnState = enuDisconnected;
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED){
      LOC_enuConnState = enuConnected;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void app_main(void)
{
  vidInitLocalVar();

  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());

  ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
  wifi_init_sta();

  TicH_vidInit();
  #if (defined(USE_EMON_CMS)&& (USE_EMON_CMS==1))
  EmonClient_vidInit();
  #endif
  #if (defined(USE_MQTT_CLIENT)&& (USE_MQTT_CLIENT==1))
  MqttSub_vidInit();
  #endif

  /* Create the periodic timer */
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  /* Start the timer */
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, u64TIMER_TIMEOUT_SECONDS));

}

static void vidInitLocalVar(void)
{
  LOC_enuConnState = enuDisconnected;
}

/********************************/

static void periodic_timer_callback(void* arg)
{
  tsrtTicInfo_t strTicInfo;
  tstrMqttSub_TicTopicsValue strTicTopic;

  // int64_t time_since_boot = esp_timer_get_time();
  // ESP_LOGI(TAG, "%lld - periodic_timer_callback()", time_since_boot);
  // ESP_LOGI(TAG, "#### periodic_timer_callback() ####");

  TicH_vidPollInfo();
  TicH_vidGetTicInfo(&strTicInfo);

  if(strTicInfo.bUpdatedVal){
    ESP_LOGI(TAG, "\t - HCHC=%d", strTicInfo.HCHC);
    ESP_LOGI(TAG, "\t - HCHP=%d", strTicInfo.HCHP);
    ESP_LOGI(TAG, "\t - IINST=%d", strTicInfo.IINST);
    ESP_LOGI(TAG, "\t - PAPP=%d", strTicInfo.PAPP);
    ESP_LOGI(TAG, "\t - PTEC=%d", (int)strTicInfo.PTEC);

#if (defined(USE_MQTT_CLIENT)&& (USE_MQTT_CLIENT==1))
    if(LOC_enuConnState == enuConnected){
      // MqttSub_vidTest(strTicInfo.IINST);
      strTicTopic.HCHC = strTicInfo.HCHC;
      strTicTopic.HCHP = strTicInfo.HCHP;
      strTicTopic.IINST = strTicInfo.IINST;
      strTicTopic.PAPP = strTicInfo.PAPP;
      strTicTopic.PTEC = (int)strTicInfo.PTEC;
      MqttSub_vidSetTicInfo(&strTicTopic);
    }
#endif

  }

#if (defined(USE_EMON_CMS)&& (USE_EMON_CMS==1))
  EmonClient_vidTest();
#endif

}
