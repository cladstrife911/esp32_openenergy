

#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "rom/ets_sys.h"

#include "assert.h"

#include "ScreenManager.h"

#include "ssd1306.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include "ssd1306_default_if.h"

/***************** LOCAL MACROS *****************/

#define USE_I2C_DISPLAY

#if defined USE_I2C_DISPLAY
    static const int I2CDisplayAddress = 0x3C;
    static const int I2CDisplayWidth = 128;
    static const int I2CDisplayHeight = 32;
    static const int I2CResetPin = -1;

    static struct SSD1306_Device I2CDisplay;
#endif

#define PROGRESS_BAX_MAX_WIDTH 90
#define PROGRESS_BAX_MAX_HEIGHT 15
#define PROGRESS_BAX_MAX_VALUE 6000


#define TASK_DELAY (200*1000)

/***************** LOCAL TYPES *****************/

/***************** LOCAL FUNCTIONS *****************/

static void vidInitLocalVar(void);
static bool DefaultBusInit( void );
static void SetupDemo( struct SSD1306_Device* DisplayHandle, const struct SSD1306_FontDef* Font );
static void SayHello( struct SSD1306_Device* DisplayHandle, const char* HelloText );
static void vidTestScreen(struct SSD1306_Device* DisplayHandle);
static void vidClearLocalizedText(struct SSD1306_Device* DisplayHandle, TextAnchor Anchor, const char* Text);

/***************** LOCAL VAR *****************/

static const char *TAG = "[ScreenMgr]";

/***************** GLOBAL FUNCTIONS *****************/
void ScreenMgr_vidInit(void)
{
  vidInitLocalVar();

  if ( DefaultBusInit( ) == true ) {
      printf( "BUS Init looking good...\n" );
      printf( "Drawing.\n" );

      #if defined USE_I2C_DISPLAY
          SetupDemo( &I2CDisplay, &Font_droid_sans_mono_7x13 );
          // SayHello( &I2CDisplay, "Hello i2c!" );
      #endif

      vidTestScreen(&I2CDisplay);

      printf( "Done!\n" );
  }

}

void ScreenMgr_vidTest(void)
{
  static int i = 0;

  i+=PROGRESS_BAX_MAX_VALUE/200;

  ScreenMgr_vidPrintNumber(i++);

  if(PROGRESS_BAX_MAX_VALUE/4 <= i){
    ScreenMgr_vidPrintHPHC(true);
  }else if(PROGRESS_BAX_MAX_VALUE/2 >= i){
    ScreenMgr_vidPrintHPHC(false);
  }

  if(i>=PROGRESS_BAX_MAX_VALUE)
    i=0;
}

/* Print the apparent power "PAPP" along with a load bar
* \param number from 0 to 6KW, -1 when error*/
void ScreenMgr_vidPrintNumber(int number)
{
  char pStringToSend[5];
  char *pMaxStringToSend="9999";
  if(number > PROGRESS_BAX_MAX_VALUE){
    number = PROGRESS_BAX_MAX_VALUE;
  }

  /*need to clear the max size len to avoid the msb to remain on the screen while not used*/
  vidClearLocalizedText(&I2CDisplay, TextAnchor_NorthEast, pMaxStringToSend);

  if(-1!=number)
  {
    sprintf(pStringToSend, "%d", number);
  }else{
    sprintf(pStringToSend, "ERR");
  }

  SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_NorthEast, pStringToSend, SSD_COLOR_WHITE );

  /*draw a "load bar" on top left to represent the PAPP*/
  /*first the border*/
  SSD1306_DrawBox(&I2CDisplay,0 ,0 ,PROGRESS_BAX_MAX_WIDTH  ,PROGRESS_BAX_MAX_HEIGHT , SSD_COLOR_WHITE, false);
  /*then the content*/
  if(-1==number)
  {
    number=0;
  }
    /*the active part*/
  SSD1306_DrawBox(&I2CDisplay,1 ,\
                              1 ,\
                              ((int)number*(PROGRESS_BAX_MAX_WIDTH-1)/PROGRESS_BAX_MAX_VALUE),\
                              PROGRESS_BAX_MAX_HEIGHT-1 , \
                              SSD_COLOR_WHITE,\
                              true);
    /*the inactive part*/
  SSD1306_DrawBox(&I2CDisplay,1+((int)number*(PROGRESS_BAX_MAX_WIDTH-1)/PROGRESS_BAX_MAX_VALUE),\
                              1 ,\
                              PROGRESS_BAX_MAX_WIDTH-1,\
                              PROGRESS_BAX_MAX_HEIGHT-1,\
                              SSD_COLOR_BLACK,\
                              true);

  SSD1306_Update( &I2CDisplay );
}

void ScreenMgr_vidPrintNetworkStatus(bool bNetworkStatus)
{
  char pStringToSend[8];

  if(bNetworkStatus){
    sprintf(pStringToSend, "Wifi OK");
  }else{
    sprintf(pStringToSend, "Wifi KO");
  }

  vidClearLocalizedText(&I2CDisplay, TextAnchor_SouthEast, pStringToSend);
  SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_SouthEast, pStringToSend, SSD_COLOR_WHITE );
  SSD1306_Update( &I2CDisplay );
}

void ScreenMgr_vidPrintNetworkIP(char *ipAddr)
{
  char *pMaxStringToSend="255.255.255.255";
  vidClearLocalizedText(&I2CDisplay, TextAnchor_NorthWest, pMaxStringToSend);
  SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_NorthWest, ipAddr, SSD_COLOR_WHITE );
  SSD1306_Update( &I2CDisplay );
}

/*Print HP (Heures Pleines) or HC (Heures creuses) on bottom left
* \param bIsHp to true when in HP, false for HC
*/
void ScreenMgr_vidPrintHPHC(bool bIsHp)
{
  char pStringToSend[3];

  if(bIsHp)
  {
    sprintf(pStringToSend, "HP");
  }else{
    sprintf(pStringToSend, "HC");
  }

  vidClearLocalizedText(&I2CDisplay, TextAnchor_SouthWest, pStringToSend);
  SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_SouthWest, pStringToSend, SSD_COLOR_WHITE );
  SSD1306_Update( &I2CDisplay );
}

/***************** LOCAL FUNCTIONS *****************/
static void vidInitLocalVar(void)
{

}

/* only clear part of the screen where the text is located */
static void vidClearLocalizedText(struct SSD1306_Device* DisplayHandle, TextAnchor Anchor, const char* Text)
{
  int x1, x2 = 0;
  int y1, y2 = 0;
  SSD1306_FontGetAnchoredStringCoords( DisplayHandle, &x1, &y1, Anchor, Text);
  y2 = SSD1306_FontGetCharHeight(DisplayHandle);
  x2 = SSD1306_FontGetCharWidth(DisplayHandle, Text);

  switch(Anchor)
  {
    case TextAnchor_SouthWest:
    {
        // printf("%d, %d, %d, %d\r\n", x1, y1, x2, y2);
        SSD1306_DrawBox(DisplayHandle,0 , DisplayHandle->Height-y2-1 , x2*2  , DisplayHandle->Height-1 , SSD_COLOR_BLACK, true);
    }break;

    case TextAnchor_NorthWest:
    {
        SSD1306_DrawBox(DisplayHandle,0 ,0 ,PROGRESS_BAX_MAX_WIDTH  ,y2 , SSD_COLOR_BLACK, true);
    }break;

    case TextAnchor_NorthEast: {
      SSD1306_DrawBox(DisplayHandle,PROGRESS_BAX_MAX_WIDTH+1 ,0 ,DisplayHandle->Width-1  ,y2 , SSD_COLOR_BLACK, true);
    }
    break;

    case TextAnchor_SouthEast: {
      SSD1306_DrawBox(DisplayHandle, x1, DisplayHandle->Height-y2-1 , DisplayHandle->Width-1 , DisplayHandle->Height-1 , SSD_COLOR_BLACK, true);

    }
    break;

    default:
      SSD1306_Clear( DisplayHandle, SSD_COLOR_BLACK );
    break;
  }

}


static bool DefaultBusInit( void ) {
    #if defined USE_I2C_DISPLAY
        assert( SSD1306_I2CMasterInitDefault( ) == true );
        assert( SSD1306_I2CMasterAttachDisplayDefault( &I2CDisplay, I2CDisplayWidth, I2CDisplayHeight, I2CDisplayAddress, I2CResetPin ) == true );
    #endif

    return true;
}

static void SetupDemo( struct SSD1306_Device* DisplayHandle, const struct SSD1306_FontDef* Font ) {
    SSD1306_Clear( DisplayHandle, SSD_COLOR_BLACK );
    SSD1306_SetFont( DisplayHandle, Font );
}

static void SayHello( struct SSD1306_Device* DisplayHandle, const char* HelloText ) {
    SSD1306_FontDrawAnchoredString( DisplayHandle, TextAnchor_Center, HelloText, SSD_COLOR_WHITE );
    SSD1306_Update( DisplayHandle );

    // SSD1306_Clear(DisplayHandle, SSD_COLOR_WHITE );
}

static void vidTestScreen(struct SSD1306_Device* DisplayHandle)
{
  static int i=0;
  i++;
  ESP_LOGI(TAG, "#### vidTestScreen() ####");

  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNetworkIP("255.255.255.255");
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintHPHC(false);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintHPHC(true);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNetworkStatus(false);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNetworkStatus(true);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNumber(-1);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNumber(0);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNumber(PROGRESS_BAX_MAX_VALUE/4);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNumber(PROGRESS_BAX_MAX_VALUE/2);
  ets_delay_us(TASK_DELAY);
  ScreenMgr_vidPrintNumber(PROGRESS_BAX_MAX_VALUE);
  ets_delay_us(TASK_DELAY);

  SSD1306_Clear( DisplayHandle, SSD_COLOR_BLACK );
}
