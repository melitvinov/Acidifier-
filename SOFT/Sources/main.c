/***********************************************************************************
* ������������ : ������ "Acidifier"
*-----------------------------------------------------------------------------------
* ������				: 1.0
* �����					: ��������
* ���� ��������			: 09.03.2021
************************************************************************************/

//--- INCLUDES -------------------

#include "FreeRTOS.h"
#include "task.h"

#include "Leds.h"
#include "AddrSwitch.h"
#include "AnaInputs.h"
#include "Modbus.h"
#include "leds-dig.h"
#include "Regulator.h"

// --- TYPES ---------------------

//--- CONSTANTS ------------------
const uint16_t DEFAULT_SETUP_PH = 50;
const float MIN_VALUE_SETUP_PH = 20;
const float MAX_VALUE_SETUP_PH = 100;

//--- GLOBAL VARIABLES -----------
uint8_t g_DeviceAddr = 0;	// ������� ����� ���������� �� ���� RS-485

EWorkMode g_WorkMode;				// ������� ����� ������ / �����������
bool g_isNoWater = false;
bool g_isErrRegulator = false;
bool g_isErrTimeoutSetupPh = false;
bool g_isErrSensors = false;

float g_Sensor_PH;					// ������� �������� PH � ��������
float g_Setup_PH;					// �������� ������������� �������� PH

bool g_isBtnPlusClick;
bool g_isBtnMinusClick;
bool g_isDblBtnPressed;

//--- IRQ ------------------------

//--- FUNCTIONS ------------------
void Thread_WORK( void *pvParameters );

int ReadWorkMode( uint16_t idx )
{
	return g_WorkMode;
}

/*******************************************************
�������		: ��������� ������ ����������
�������� 1	: ���
�����. ����.: ����� ����������
********************************************************/
uint8_t GetDeviceAddress(void)
{
	return g_DeviceAddr;
}

/*******************************************************
�����		: ���������� ��������� ������ �� ���� RS-485
�������� 1	: �� ������������
�����. ����.: ����������� ����
********************************************************/
void CheckAddrChange( void *pvParameters )
{
	for(;;)
	{
		uint16_t addr = ADRSW_GetAdr();
		if( addr != g_DeviceAddr )
		{
			vTaskDelay(2000);
			if( addr == ADRSW_GetAdr() )
			{
				// ����� ���������� ���������, ������
				g_DeviceAddr = addr;
			}
		}
	}
}

/*******************************************************
	�������	����������� ����� ������
********************************************************/
//void SwitchWorkMode( EWorkMode newWorkMode )
//{
//	g_WorkMode = newWorkMode;
//	
//	Leds_OffAll();
//	
//	switch( g_WorkMode )
//	{
//		case Mode_RegulatorPh:
//			break;
//			
//		case Mode_Calibrating_PH1:
//			Reg_RelayAllOff();
//			Led_On( LED_TAR_P1 );
//			break;
//		
//		case Mode_Calibrating_PH2:
//			Reg_RelayAllOff();
//			Led_On( LED_TAR_P2 );
//			break;
//	}
//}

bool _isBtnPlusPressed( void )
{
	return (bool) (GPIO_PinRead( PORT_BTN_PLUS, PIN_BTN_PLUS )==0);
}
bool _isBtnMinusPressed( void )
{
	return (bool) (GPIO_PinRead( PORT_BTN_MINUS, PIN_BTN_MINUS )==0);
}

/*******************************************************
�����		: ������� ����� ����������
�������� 1	: �� ������������
�����. ����.: ����������� ����
********************************************************/
void Thread_Buttons( void *pvParameters )
{
	const int TIME_MS = 25; 
	const int TIME_MAXMS_BTNDOWN = 500; 
	const int TIME_DBLBTN = 2000; 

	bool isBtnPlusPressedPrev = false;
	bool isBtnMinusPressedPrev = false;
	bool isBtnPlusPressedNow;
	bool isBtnMinusPressedNow;
	int msBtnPlusPressed = 0;
	int msBtnMinusPressed = 0;
	bool skipNextUpPlus = false;
	bool skipNextUpMinus = false;
	bool skipDblButtons = false;
	
	GPIO_PinConfigure( PORT_BTN_PLUS, PIN_BTN_PLUS, GPIO_IN_PULL_UP, GPIO_MODE_INPUT );
	GPIO_PinConfigure( PORT_BTN_MINUS, PIN_BTN_MINUS, GPIO_IN_PULL_UP, GPIO_MODE_INPUT );
	
	for(;;)
	{
		isBtnPlusPressedNow = _isBtnPlusPressed();
		isBtnMinusPressedNow = _isBtnMinusPressed();
		
		if( !isBtnPlusPressedNow )
		{
			if( isBtnPlusPressedPrev ) 
			{
				if( !skipNextUpPlus )
				{
					if( msBtnPlusPressed < TIME_MAXMS_BTNDOWN )
					g_isBtnPlusClick = true;
				}
				else
				{
					// ��� ���������� ������ ������������ ������ ������� ���� ������
					skipDblButtons = false;
				}
				skipNextUpPlus = false;
			}
			msBtnPlusPressed = 0;
		}
		else
		{
			if( isBtnPlusPressedPrev ) 
			{
				msBtnPlusPressed += TIME_MS;
				if(( msBtnPlusPressed > TIME_DBLBTN ) && ( msBtnMinusPressed > TIME_DBLBTN ) && !skipDblButtons )
				{
					g_isDblBtnPressed = true;
					skipNextUpPlus = true;
					skipNextUpMinus = true;
					skipDblButtons = true;
				}
			}
		}
		
		if( !isBtnMinusPressedNow )
		{
			if( isBtnMinusPressedPrev ) 
			{
				if( !skipNextUpMinus )
				{
					if( msBtnMinusPressed < TIME_MAXMS_BTNDOWN )
					g_isBtnMinusClick = true;
				}
				else
				{
					// ��� ���������� ������ ������������ ������ ������� ���� ������
					skipDblButtons = false;
				}
				skipNextUpMinus = false;
			}
			msBtnMinusPressed = 0;
		}
		else
		{
			if( isBtnMinusPressedPrev ) 
			{
				msBtnMinusPressed += TIME_MS;
				if(( msBtnPlusPressed > TIME_DBLBTN ) && ( msBtnMinusPressed > TIME_DBLBTN ) && !skipDblButtons )
				{
					g_isDblBtnPressed = true;
					skipNextUpPlus = true;
					skipNextUpMinus = true;
					skipDblButtons = true;
				}
			}
		}
		
		isBtnMinusPressedPrev = isBtnMinusPressedNow;
		isBtnPlusPressedPrev = isBtnPlusPressedNow;
		
		vTaskDelay(TIME_MS);
	}
}

/*******************************************************
�������		: ������������� ����������
�������� 1	: ���
�����. ����.: ���
********************************************************/
void Initialize()
{
	Leds_init();
	ADRSW_init();
	
	g_DeviceAddr = 0;
}

/*******************************************************
�������		: ����� ������ ����������
�������� 1	: ���
�����. ����.: ����������� ����
********************************************************/
int main(void)
{
	Initialize();

	xTaskCreate( CheckAddrChange, (const char*)"ADDRESS", configMINIMAL_STACK_SIZE,	( void * ) NULL, ( tskIDLE_PRIORITY + 1 ), NULL);

	xTaskCreate( AInp_Thread, (const char*)"ANALOG", configMINIMAL_STACK_SIZE,	( void * ) NULL, ( tskIDLE_PRIORITY + 1 ), NULL);

	xTaskCreate( MBUS_Thread, (const char*)"Modbus", 0x1000,	( void * ) NULL, ( tskIDLE_PRIORITY + 2 ), NULL);

	xTaskCreate( Thread_Leds_Dig, (const char*)"LedsDig", configMINIMAL_STACK_SIZE,	( void * ) NULL, ( tskIDLE_PRIORITY + 1 ), NULL);
	
	xTaskCreate( Thread_Regulator, (const char*)"Regulator", configMINIMAL_STACK_SIZE,	( void * ) NULL, ( tskIDLE_PRIORITY + 1 ), NULL);
	
	xTaskCreate( Thread_Buttons, (const char*)"BTN", configMINIMAL_STACK_SIZE,	( void * ) NULL, ( tskIDLE_PRIORITY + 1 ), NULL);

	xTaskCreate( Thread_WORK, (const char*)"WORK", configMINIMAL_STACK_SIZE,	( void * ) NULL, ( tskIDLE_PRIORITY + 1 ), NULL);
	/* Start the scheduler. */
	vTaskStartScheduler();

}

/*******************************************************
	������ ������ �������� PH � EEPROM
********************************************************/
bool WriteSetupPhValue( uint16_t idx, uint16_t val )
{
	if(( val < MIN_VALUE_SETUP_PH ) || ( val > MAX_VALUE_SETUP_PH ))
		return false;
	
	if( FM24_WriteBytes( EEADR_SETUP_PH, (uint8_t*) &val, sizeof( uint16_t ) ) )
	{
		// ���������� � ���������� ����������
		g_Setup_PH = val;
		g_Setup_PH /= 10.0;
		
		return true;
	}
	return false;

}
/*******************************************************
	������ �������� ������� PH �� EEPROM
********************************************************/
int ReadSetupPhValue( uint16_t idx )
{
	uint16_t uiValue;
	
	if( !FM24_ReadBytes( EEADR_SETUP_PH, (uint8_t*) &uiValue, sizeof( uint16_t ) ) )
		return -1;
	
	if(( uiValue < MIN_VALUE_SETUP_PH ) || ( uiValue > MAX_VALUE_SETUP_PH ))
	{
		// �������� �������������� PH ����� ��� �������� ���������
		// ������������� �������� �� ���������
		uiValue = DEFAULT_SETUP_PH;
		WriteSetupPhValue( 0, uiValue );
	}

	g_Setup_PH = uiValue;
	g_Setup_PH /= 10.0;
	return uiValue;
}

/*******************************************************
	��������� ������ �������� PH
********************************************************/
bool SetupPhValue( float value )
{
	// ��� ���� ��������� ����� �������� ��������� PH
	value *= 10.0;
	uint16_t uiValue  = roundf( value );
	return WriteSetupPhValue( 0, uiValue );
}

/*******************************************************
�����		: ������� ����� ����������
�������� 1	: �� ������������
�����. ����.: ����������� ����
********************************************************/
void Thread_WORK( void *pvParameters )
{
	float ph1, ph2;
	
	//g_Status = 0;

	ReadSetupPhValue(0);
	//SwitchWorkMode( Mode_RegulatorPh );
	
	g_WorkMode = Mode_RegulatorPh;
	
	for( int i=0; i<20; i++ )
	{
		Led_On( LED_SYS );
		vTaskDelay(100);
		Led_Off( LED_SYS );
		vTaskDelay(100);
	}
	
	vTaskDelay(2000);

	for(;;)
	{
		vTaskDelay(50);
		
		bool stopWork = g_isErrRegulator || g_isErrSensors || g_isErrTimeoutSetupPh || g_isNoWater;
		g_isErrRegulator ? Led_On( LED_ERR_REGULATOR ) : Led_Off( LED_ERR_REGULATOR );
		g_isErrSensors ? Led_On( LED_ERR_SENSORS ) : Led_Off( LED_ERR_SENSORS );
		g_isErrTimeoutSetupPh ? Led_On( LED_ERR_SETUP_PH_TIMEOUT ) : Led_Off( LED_ERR_SETUP_PH_TIMEOUT );
		g_isNoWater ? Led_On( LED_NO_WATER ) : Led_Off( LED_NO_WATER );
		
		switch( (int)g_WorkMode )
		{
			case Mode_RegulatorPh:

				Led_Off( LED_TAR_P1 );
				Led_Off( LED_TAR_P2 );
				
				if( stopWork )
				{
					Led_Off( LED_WORK_OK );
					LcdDig_DispBlinkOn( SideLEFT );
					LcdDig_DispBlinkOff( SideRIGHT );
				}
				else
				{
					Led_On( LED_WORK_OK );
					LcdDig_DispBlinkOff( SideLEFT );
					LcdDig_DispBlinkOff( SideRIGHT );
				}
				if( g_isDblBtnPressed )
				{
					g_isDblBtnPressed = false;
					g_isBtnPlusClick = false;
					g_isBtnMinusClick = false;
					g_WorkMode = Mode_Calibrating_PH1;
				}
				else if( g_isBtnPlusClick )
				{
					g_isBtnPlusClick = false;
					
					if( g_Setup_PH + 0.1 < 10 )
						SetupPhValue( g_Setup_PH + 0.1 );
				}
				else if( g_isBtnMinusClick )
				{
					g_isBtnMinusClick = false;
					
					if( g_Setup_PH - 0.1 > 0 )
						SetupPhValue( g_Setup_PH - 0.1 );
				}
				break;
			
			case Mode_Calibrating_PH1:
				
				Led_Off( LED_WORK_OK );
				Led_On( LED_TAR_P1 );
				Led_Off( LED_TAR_P2 );

				ph1 = AInp_GetFloatSensorPh(0);
				ph2 = AInp_GetFloatSensorPh(1);
				LcdDig_PrintPH( ph1, SideLEFT, false );
				LcdDig_PrintPH( ph2, SideRIGHT, false );
			
				if( g_isDblBtnPressed )
				{
					// ����� ������� ������
					g_isDblBtnPressed = false;
					g_isBtnPlusClick = false;
					g_isBtnMinusClick = false;
					
					// ������������� ���������� �����
					// ����������� �������������
					LcdDig_DispBlinkOn( SideLEFT | SideRIGHT );
					vTaskDelay( 1500 );
					LcdDig_DispBlinkOff( SideLEFT | SideRIGHT );
					// ������ ����������
					AInp_WriteAdcTar1( 0, AInp_ReadAdcValue( 0 ) );
					AInp_WriteAdcTar1( 1, AInp_ReadAdcValue( 1 ) );
					
					g_WorkMode = Mode_Calibrating_PH2;
				}
				else if( g_isBtnPlusClick || g_isBtnMinusClick )
				{
					g_isBtnPlusClick = false;
					g_isBtnMinusClick = false;
					
					g_WorkMode = Mode_RegulatorPh;
				}
			
				break;
				
			case Mode_Calibrating_PH2:
				Led_Off( LED_WORK_OK );
				Led_On( LED_TAR_P2 );
				Led_Off( LED_TAR_P1 );

				ph1 = AInp_GetFloatSensorPh(0);
				ph2 = AInp_GetFloatSensorPh(1);
				LcdDig_PrintPH( ph1, SideLEFT, false );
				LcdDig_PrintPH( ph2, SideRIGHT, false );
			
				if( g_isDblBtnPressed )
				{
					g_isDblBtnPressed = false;
					g_isBtnPlusClick = false;
					g_isBtnMinusClick = false;
					
					// ����� ������� ������
					g_isDblBtnPressed = false;
					g_isBtnPlusClick = false;
					g_isBtnMinusClick = false;
					
					// ������������� ���������� �����
					// ����������� �������������
					LcdDig_DispBlinkOn( SideLEFT | SideRIGHT );
					vTaskDelay( 1500 );
					LcdDig_DispBlinkOff( SideLEFT | SideRIGHT );
					// ������ ����������
					AInp_WriteAdcTar2( 0, AInp_ReadAdcValue( 0 ) );
					AInp_WriteAdcTar2( 1, AInp_ReadAdcValue( 1 ) );
					
					g_WorkMode = Mode_RegulatorPh;
				}
				else if( g_isBtnPlusClick || g_isBtnMinusClick )
				{
					g_isBtnPlusClick = false;
					g_isBtnMinusClick = false;
					
					g_WorkMode = Mode_RegulatorPh;
				}
				
				break;
			
			default:
				g_WorkMode = Mode_RegulatorPh;
		}
	}
}
