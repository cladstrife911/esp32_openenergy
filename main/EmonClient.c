

#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "assert.h"

#include "EmonClient.h"

#include "esp_http_client.h"

/***************** LOCAL MACROS *****************/

#define POST_DATA_SIZE 256

/***************** LOCAL TYPES *****************/

/***************** LOCAL FUNCTIONS *****************/

static void vidInitLocalVar(void);
static void vidPostTest(char* au8ValueToSend);
static esp_err_t enuHttpEvent_callback(esp_http_client_event_t *evt);


/***************** LOCAL VAR *****************/

static const char *TAG = "[Emon]";

/*** HTTP params ***/
static esp_http_client_config_t LOC_strHttpConfig = {
    .url = "http://192.168.1.2/emoncms",
    // .url = "http://192.168.1.2/emoncms/input/post?node=emontx&json={power1:100,power2:200,power3:300}",
    .event_handler = enuHttpEvent_callback,
};
static esp_http_client_handle_t LOC_HttpClient;

/***************** GLOBAL FUNCTIONS *****************/
void EmonClient_vidInit(void)
{
  vidInitLocalVar();
}

void EmonClient_vidTest(void)
{
  char *pStringToSend="power1:100,power2:200,power3:300";
  vidPostTest(pStringToSend);
}


/***************** LOCAL FUNCTIONS *****************/
static void vidInitLocalVar(void)
{

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


static esp_err_t enuHttpEvent_callback(esp_http_client_event_t *evt)
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
