

#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

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

/***************** LOCAL TYPES *****************/

/***************** LOCAL FUNCTIONS *****************/

static void vidInitLocalVar(void);
static bool DefaultBusInit( void );
static void SetupDemo( struct SSD1306_Device* DisplayHandle, const struct SSD1306_FontDef* Font );
static void SayHello( struct SSD1306_Device* DisplayHandle, const char* HelloText );
static void vidTestScreen(struct SSD1306_Device* DisplayHandle);
static void ScreenMgr_vidPrintNumber(int number);
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
          SayHello( &I2CDisplay, "Hello i2c!" );
      #endif

      // while(1){
      //   vTaskDelay(100);
      //   vidTestScreen(&I2CDisplay);
      // }
      printf( "Done!\n" );
  }

}

void ScreenMgr_vidTest(void)
{
  static int i =0;
  ScreenMgr_vidPrintNumber(i++%150);
}

void ScreenMgr_vidPrintNumber(int number)
{
  // int x = 0;
  // int y = 0;

  char pStringToSend[10];
  char *pMaxStringToSend="9999";
  sprintf(pStringToSend, "%d", number);

  /*need to clear the max size len to avoid the msb to remain on the screen while not used*/
  vidClearLocalizedText(&I2CDisplay, TextAnchor_NorthEast, pMaxStringToSend);
  SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_NorthEast, pStringToSend, SSD_COLOR_WHITE );
  SSD1306_Update( &I2CDisplay );
}

void ScreenMgr_vidPrintNetworkStatus(bool bNetworkStatus)
{
  // int x = 0;
  // int y = 0;

  char pStringToSend[10];


  if(bNetworkStatus){
    sprintf(pStringToSend, "Wifi OK");
  }else{
    sprintf(pStringToSend, "Wifi KO");
  }

  //only refresh the screen where the number is supposed to be
  // SSD1306_FontGetAnchoredStringCoords( &I2CDisplay, &x, &y, TextAnchor_SouthEast, pStringToSend);
  // y = SSD1306_FontGetCharHeight( &I2CDisplay );

  // printf("x=%d, y=%d", x, y);
  // SSD1306_DrawBox(&I2CDisplay,x ,I2CDisplay.Height-y ,I2CDisplay.Width ,I2CDisplay.Height , SSD_COLOR_BLACK, true);
  vidClearLocalizedText(&I2CDisplay, TextAnchor_SouthEast, pStringToSend);

  SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_SouthEast, pStringToSend, SSD_COLOR_WHITE );
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
  y2 = SSD1306_FontGetCharHeight( DisplayHandle );

  switch(Anchor)
  {
    case TextAnchor_NorthEast: {
      SSD1306_DrawBox(DisplayHandle,x1 ,0 ,DisplayHandle->Width  ,y2 , SSD_COLOR_BLACK, true);
    }
    break;

    case TextAnchor_SouthEast: {
      SSD1306_DrawBox(DisplayHandle, x1, DisplayHandle->Height-y2 , DisplayHandle->Width , DisplayHandle->Height , SSD_COLOR_BLACK, true);

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
}

static void vidTestScreen(struct SSD1306_Device* DisplayHandle)
{
  static int i=0;
  i++;
  ESP_LOGI(TAG, "#### vidTestScreen() ####");
  if(DisplayHandle!=NULL){
    SSD1306_Clear(DisplayHandle, SSD_COLOR_BLACK );
    SSD1306_DrawVLine(DisplayHandle,i%128,(i+1)%128,i%32, SSD_COLOR_WHITE);
    SSD1306_Update( DisplayHandle );
  }
}
