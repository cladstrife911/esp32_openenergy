#ifndef _SCREEN_MANAGER_H
#define _SCREEN_MANAGER_H


extern void ScreenMgr_vidInit(void);
extern void ScreenMgr_vidTest(void);
extern void ScreenMgr_vidPrintNetworkStatus(bool bNetworkStatus);
extern void ScreenMgr_vidPrintNetworkIP(char *ipAddr);
extern void ScreenMgr_vidPrintHPHC(bool bIsHp);
extern void ScreenMgr_vidPrintNumber(int number);

#endif
