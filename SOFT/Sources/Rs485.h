/***********************************************************************************
* ������������ ���� ���	: ������ ����� �� ���� RS-485
*
************************************************************************************/

#ifndef __RS485_H
#define __RS485_H

//--- INCLUDES -------------------

#include "GPIO_STM32F10x.h"
#include "stm32f10x.h"
#include "Board.h"

// --- DEFINES -------------------
#define DELAY_BEFORE_SEND	1	// �������� �������� ����� ������������ � ����� ��������
#define DELAY_AFTER_SEND	2	// �������� ������������ � ����� ������ ����� ��������

// --- TYPES ---------------------

//--- FUNCTIONS ------------------

void Rs485_Init(void);
void RS485_SendBuf( const char * pBuf, uint8_t count );

#endif
