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

#include "driver/uart.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_http_client.h"

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
#define u64TIMER_TIMEOUT_SECONDS (5*MICROSEC_TO_SEC)

#define POST_DATA_SIZE 256

/****************** local functions declarations *****************/
static void periodic_timer_callback(void* arg);
static void vidPostTest(char* au8ValueToSend);
static esp_err_t _http_event_handler(esp_http_client_event_t *evt);
static void vidInitUart(void);

/****************** local variables *****************/
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };
static esp_timer_handle_t periodic_timer;

/*** HTTP params ***/
static esp_http_client_config_t LOC_strHttpConfig = {
    .url = "http://192.168.1.2/emoncms",
    // .url = "http://192.168.1.2/emoncms/input/post?node=emontx&json={power1:100,power2:200,power3:300}",
    .event_handler = _http_event_handler,
};
static esp_http_client_handle_t LOC_HttpClient;

/*** UART params ***/
#define UART_TXD  (GPIO_NUM_4)
#define UART_RXD  (GPIO_NUM_5)
const int uart_num = UART_NUM_1;
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};
// Setup UART buffered IO with event queue
const int uart_buffer_size = (255);
QueueHandle_t uart_queue;
// static char* test_str = "Echo\r\n";
static uint8_t uart_in_data[255];
static int uart_in_length = 0;

static char buffer_util_data[128];

/****************** local functions definitions *****************/

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
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


	/* Create the periodic timer */
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	/* Start the timer */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, u64TIMER_TIMEOUT_SECONDS));
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    vidInitUart();

}

/********************************/

static void periodic_timer_callback(void* arg)
{


  int64_t time_since_boot = esp_timer_get_time();
  ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);

  // Loopback test - Write data to UART.
  // uart_write_bytes(uart_num, (const char*)test_str, strlen(test_str));

  // Read data from UART.
  ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&uart_in_length));
  uart_in_length = uart_read_bytes(uart_num, uart_in_data, uart_in_length, 0);
  // ESP_LOGI(TAG, "Uart RX size=%d", uart_in_length);
  // ESP_LOGI(TAG, "Uart RX data=%s", uart_in_data);

  for(int i=0;i<uart_in_length; i++)
  {
    if(uart_in_data[i]=='\r')
    {
      memcpy(buffer_util_data,uart_in_data, i);
      buffer_util_data[i]='\0'; /*end of string char*/
      i=uart_in_length; /*Exit for loop*/
    }
  }
  ESP_LOGI(TAG, "buffer_util_data=%s", buffer_util_data);

  if(strlen(buffer_util_data) != 0){
    vidPostTest(buffer_util_data);
  }

  uart_flush(uart_num);
}

/*
* GET:  http://192.168.1.2/emoncms/input/post?node=emontx&fulljson={"power1":100,"power2":200,"power3":300}
* POST:  curl --data "node=1&data={power1:100,power2:200,power3:300}&apikey=c2ab2cb10bbb69ff568cb4e8cf70c198" "http://192.168.1.2/emoncms/input/post"
*/
static void vidPostTest(char* au8ValueToSend)
{
  esp_err_t err;
  char post_data[POST_DATA_SIZE];

  // sprintf(au8ValueToSend, "Irms1:10.0,Irms2:25.0");

  LOC_HttpClient = esp_http_client_init(&LOC_strHttpConfig);

  /*** POST ***/
  if(au8ValueToSend != NULL)
  {
    sprintf(post_data,
            "node=esp32&data={%s}&apikey=c2ab2cb10bbb69ff568cb4e8cf70c198",
            au8ValueToSend);
    ESP_LOGI(TAG, "POST data:%s",post_data);
    esp_http_client_set_url(LOC_HttpClient, "http://192.168.1.2/emoncms/input/post");
    esp_http_client_set_method(LOC_HttpClient, HTTP_METHOD_POST);
    esp_http_client_set_post_field(LOC_HttpClient, post_data, strlen(post_data));
    err = esp_http_client_perform(LOC_HttpClient);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(LOC_HttpClient),
                esp_http_client_get_content_length(LOC_HttpClient));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
  }
  else{
    ESP_LOGE(TAG, "input char is not valid");
  }

  /*Must be called in the same fct as esp_http_client_init*/
  esp_http_client_cleanup(LOC_HttpClient);
}

static void vidInitUart(void)
{
  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  // Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: IO18, CTS: IO19)
  ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TXD, UART_RXD, 18, 19));
  // Install UART driver using an event queue here
  ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, \
                                      uart_buffer_size, 10, &uart_queue, 0));
}

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            // ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            // int mbedtls_err = 0;
            // esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            // if (err != 0) {
            //     ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            //     ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            // }
            break;
    }
    return ESP_OK;
}
