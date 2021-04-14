/***********************************************************************************
* ������������ ���� ���	: ������ ��������� �����������
*
************************************************************************************/

#ifndef __LEDS_H_
#define __LEDS_H_

//--- INCLUDES -------------------

#include "GPIO_STM32F10x.h"
#include "stm32f10x.h"
#include "Board.h"

// --- DEFINES -------------------
#define TO_ON	(0) // ���������� ������� ��� ��������� ����������
#define TO_OFF	(1)	// ���������� ������� ��� ���������� ����������


// --- TYPES ---------------------

//-- ������� �����������
enum 
{
	LED_SYS = 0,
	LED_WORK_OK,
	LED_NO_WATER,
	LED_ERR_REGULATOR,
	LED_ERR_SETUP_PH_TIMEOUT,
	LED_ERR_SENSORS,
	LED_TAR_P1,
	LED_TAR_P2,
	LED_MOVE_PH_PLUS,
	LED_MOVE_PH_MINUS,
	LED_STOP_PH_PLUS,
	LED_STOP_PH_MINUS,
};

typedef struct __leddesc
{
	GPIO_TypeDef *GPIOx;
	uint16_t pin;
} TLedDesc;

//--- FUNCTIONS ------------------

void Leds_init(void);
bool Led_IsOn(uint8_t indx);
void Led_On(uint8_t indx);
void Led_Off(uint8_t indx);
void Led_Switch(uint8_t indx);
void Leds_OnAll(void);
void Leds_OffAll(void);

#endif
