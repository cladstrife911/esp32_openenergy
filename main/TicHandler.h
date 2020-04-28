#ifndef _TIC_HANDLER_H
#define _TIC_HANDLER_H

typedef enum
{
  enuTic_PeriodeHeurePleine,
  enuTic_PeriodeHeureCreuse,
  enuTic_PeriodeError,
} tenuTicPeriodeTarifaire;

typedef struct
{
  int HCHC;//Heures Creuses 9 char in Wh
  int HCHP;//Heures Pleines 9 char in Wh
  tenuTicPeriodeTarifaire PTEC;//Période Tarifaire en cours
  int IINST;//Intensité Instantanée 3 char in A
  int PAPP;//Puissance apparente 5 char in VA
}tsrtTicInfo_t;

extern void TicH_vidInit(void);
extern void TicH_vidPollInfo(void);
extern void TicH_vidGetTicInfo(tsrtTicInfo_t *pstrTicInfo);

#endif
