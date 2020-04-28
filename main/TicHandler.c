

#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "assert.h"

#include "TicHandler.h"

/***************** LOCAL MACRO *****************/
#define UART_TXD  (GPIO_NUM_4)
#define UART_RXD  (GPIO_NUM_5)
#define UART_RX_BUFF_SIZE (512)

#define ASCII_ETX (0x03)
#define ASCII_STX (0x02)

#define TIC_INFO_SIZE (180)

/***************** LOCAL TYPE *****************/
typedef enum
{
  enuStxNotFound, /*start of text*/
  enuStxFound,
  enuEtxFound, /*end of text*/
  enuError,
}tenuUartState;

/***************** LOCAL FUNCTION *****************/
static void vidInitUart(void);
static void vidInitLocalVar(void);
static void vidParseUartInfo(char *acInputData, tsrtTicInfo_t *pstrOutputTicInfo);
static void vidExtractValue(char *string, int *pOutput);

/***************** LOCAL VAR *****************/

static const char *TAG = "[TicH]";

static tsrtTicInfo_t LOC_strTicInfo;

/*** UART params ***/
const int uart_num = UART_NUM_1;
uart_config_t uart_config = {
    .baud_rate = 1200,//115200,
    .data_bits = UART_DATA_7_BITS,//UART_DATA_8_BITS,
    .parity = UART_PARITY_EVEN,//UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};

QueueHandle_t uart_queue;
// static char* test_str = "Echo\r\n";
static uint8_t LOC_au8UartRxBuff[UART_RX_BUFF_SIZE];
static int LOC_iUartRxLen = 0;


static char buffer_util_data[180];


void TicH_vidInit(void)
{
  vidInitLocalVar();
  vidInitUart();
}

/**
* @brief This function shall be called periodicaly (independent from the periodicity)
* to check the content of the UART RX buffer
* @note call TicH_vidInit before
*/
void TicH_vidPollInfo(void)
{
  static tenuUartState enuUartState = enuStxNotFound;
  static int byteCounter = 0;

  // Read data from UART.
  ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&LOC_iUartRxLen));
  ESP_LOGI(TAG, "Uart RX size=%d", LOC_iUartRxLen);
  if(LOC_iUartRxLen>=TIC_INFO_SIZE)
  {
    LOC_iUartRxLen = uart_read_bytes(uart_num, LOC_au8UartRxBuff, LOC_iUartRxLen, 0);

    for(int i=0;i<LOC_iUartRxLen; i++)
    {
      switch(enuUartState){
        case enuStxNotFound:
          if(LOC_au8UartRxBuff[i]==ASCII_STX)
          {
            enuUartState = enuStxFound;
            byteCounter = 0;
          }else{
            //continue to search STX char
          }
        break;

        case enuStxFound:
          //check for end of text
          if(LOC_au8UartRxBuff[i]==ASCII_ETX)
          {
            buffer_util_data[byteCounter] = '\0';
            enuUartState = enuEtxFound;
            byteCounter = 0;
            i=LOC_iUartRxLen; /*Exit for loop*/
          }else{
            //continue to store the data
            buffer_util_data[byteCounter] = LOC_au8UartRxBuff[i];
            byteCounter++;
            if(byteCounter>=TIC_INFO_SIZE){
              enuUartState = enuError;
            }
          }
        break;

        case enuEtxFound:
        case enuError:
        default:
          enuUartState = enuStxNotFound;
        break;
      }

    }

    //Only analyze the received bytes if the frame is complete
    if(enuUartState == enuEtxFound){
      // ESP_LOGI(TAG, "UART TIC Info raw=%s", buffer_util_data);
      vidParseUartInfo(buffer_util_data, &LOC_strTicInfo);
      LOC_strTicInfo.bUpdatedVal = true;
      LOC_strTicInfo.i64TimeLastUpdate = esp_timer_get_time();
      uart_flush(uart_num);
    }
  }else{
    LOC_strTicInfo.bUpdatedVal = false;
  }
}

/**
* @brief This function will return the current values
* if the application is polling at higher speed than refresh, the old
* value will be sent again
* @note call TicH_vidInit & TicH_vidPollInfo before
*/
void TicH_vidGetTicInfo(tsrtTicInfo_t *pstrTicInfo)
{
  assert(pstrTicInfo!=NULL);
  memcpy(pstrTicInfo, &LOC_strTicInfo, sizeof(LOC_strTicInfo));
}

/***************** LOCAL FUNCTIONS *****************/
static void vidParseUartInfo(char *acInputData, tsrtTicInfo_t *pstrOutputTicInfo)
{
  assert(acInputData!=NULL);
  assert(pstrOutputTicInfo!=NULL);

  int iVal;

  // ESP_LOGI(TAG, "vidParseUartInfo()");

  char *ptr = strstr(acInputData, "HCHC");
	if (ptr != NULL) /* Substring found */
	{
		// ESP_LOGI(TAG, "HCHC found");
    vidExtractValue(ptr+5, &(pstrOutputTicInfo->HCHC));
	}else{
    ESP_LOGI(TAG, "HCHC missing");
  }

  ptr = strstr(acInputData, "HCHP");
	if (ptr != NULL) /* Substring found */
	{
		// ESP_LOGI(TAG, "HCHP found");
    vidExtractValue(ptr+5, &(pstrOutputTicInfo->HCHP));
	}else{
    ESP_LOGI(TAG, "HCHP missing");
  }

  ptr = strstr(acInputData, "PTEC");
	if (ptr != NULL) /* Substring found */
	{
		// ESP_LOGI(TAG, "PTEC found: %c",ptr[6] );
    switch(ptr[6])
    {
      case 'P':
        // ESP_LOGI(TAG, "Heure pleine");
        pstrOutputTicInfo->PTEC = enuTic_PeriodeHeurePleine;
      break;
      case 'C':
        // ESP_LOGI(TAG, "Heure creuse");
        pstrOutputTicInfo->PTEC = enuTic_PeriodeHeureCreuse;
      break;
      default:
        // ESP_LOGI(TAG, "PTEC value error");
        pstrOutputTicInfo->PTEC = enuTic_PeriodeError;
      break;
    }

	}else{
    ESP_LOGI(TAG, "PTEC missing");
  }

  ptr = strstr(acInputData, "IINST");
	if (ptr != NULL) /* Substring found */
	{
		// ESP_LOGI(TAG, "IINST found");
    vidExtractValue(ptr+6, &(pstrOutputTicInfo->IINST));
	}else{
    ESP_LOGI(TAG, "IINST missing");
  }

  ptr = strstr(acInputData, "PAPP");
	if (ptr != NULL) /* Substring found */
	{
		// ESP_LOGI(TAG, "PAPP found");
    vidExtractValue(ptr+5, &(pstrOutputTicInfo->PAPP));
	}else{
    ESP_LOGI(TAG, "PAPP missing");
  }

}

static void vidExtractValue(char *string, int *pOutput)
{
  assert(string!=NULL);
  assert(pOutput!=NULL);

  char cTemp[10];

  for(int i=0; i<10; i++)
  {
    if(string[i+1] == ' '){
      cTemp[i] = '\0';
      i=10;//stop for loop
    }else{
      cTemp[i] = string[i+1];
    }
  }
  cTemp[9] = '\0';
  // ESP_LOGI(TAG, "cTemp=[%s]",cTemp);
  *pOutput = atoi(cTemp);
  // ESP_LOGI(TAG, "*pOutput=[%d]",*pOutput);
}

static void vidInitLocalVar(void)
{
  memset(&LOC_au8UartRxBuff, 0, sizeof(LOC_au8UartRxBuff));
  memset(&LOC_strTicInfo, 0, sizeof(LOC_strTicInfo));
  LOC_strTicInfo.bUpdatedVal = false;
}

static void vidInitUart(void)
{
  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  // Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: IO18, CTS: IO19)
  ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TXD, UART_RXD, 18, 19));
  // Install UART driver using an event queue here
  ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_RX_BUFF_SIZE, \
                                      0, 10, &uart_queue, 0));
}
