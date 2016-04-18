#include <driverlib.h>
#include <msp432.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdint.h"
#include "support.h"
#include "ST7735.h"
#include "ClockSystem.h"

char weatherString[]  = "GET http://api.wunderground.com/api/6515cde8f15218cd/conditions/q/MI/Grand_Rapids.json HTTP/1.1\r\nHost:api.wunderground.com\r\nConnection:close\r\n\r\n";
char forecastString[] = "GET http://api.wunderground.com/api/6515cde8f15218cd/forecast/q/MI/Grand_Rapids.json HTTP/1.1\r\nHost:api.wunderground.com\r\nConnection:close\r\n\r\n";
const char httpStriptOLD[] = "<form action=\"action_page.php\">\
  <form action=\"\"> \
  <input type=\"radio\" name=\"LED\" value=\"Red\"> Red<br> \
  <input type=\"radio\" name=\"LED\" value=\"Blue\"> Blue<br> \
  <input type=\"radio\" name=\"LED\" value=\"Green\"> Green \
  <br> \
  <input type=\"submit\" value=\"Submit\"> \
  </form>";

const char httpStript1[] = "<!DOCTYPE html>\
<html>\
<head>\
<style>\
header {\
    background-color:blue;\
    color:white;\
    width:600px;\
    text-align:center;\
    padding:5px;	 \
}\
nav {\
    line-height:30px;\
    background-color:#eeefff;\
    height:300px;\
    width:225px;\
    float:left;\
    padding:5px;\
}\
form {\
    width:600px;\
    height:10px;\
    background-color:#FFFFFF;\
    float:left;\
    padding:5px;\
}\
form {\
    width:600px;\
    height:40px;\
    background-color:#FFFFFF;\
    float:left;\
    padding:5px;\
}\
form {\
	width:600px;\
	height:40px;\
	background-color:#FFFFFF;\
	float:left;\
	padding:5px;\
}\
footer {\
    background-color:black;\
    color:white;\
    clear:both;\
    text-align:center;\
    padding:5px;\
}\
</style>\
</head>\
<body>\
<header>\
<h1>EGR 436 Weatherball Web Application!</h1>\
</header>\
<nav>\
EGR 436<br>\
John Howland<br>\
Tim Nyugen<br>";


const char httpStript2[] = "</nav>\
<form action=\"action_page.php\">\
  <p>LCD control:\
  <input type=\"radio\" name=\"Screen\" value=\"Today\" checked> Today\
  <input type=\"radio\" name=\"Screen\" value=\"Tomorrow\"> Tomorrow\
  <input type=\"radio\" name=\"Screen\" value=\"Scrolling\"> Scrolling\
  <input type=\"submit\" value=\"Submit\"> </p>\
</form>\
<form action=\"action_page.php\">\
  <p>LED control:\
  <input type=\"radio\" name=\"LED\" value=\"Red\" checked> Red\
  <input type=\"radio\" name=\"LED\" value=\"Blue\"> Blue\
  <input type=\"radio\" name=\"LED\" value=\"Green\"> Green\
  <input type=\"submit\" value=\"Submit\"> </p>\
</form>\
<form action=\"action_page.php\">\
  <p>Extra:\
  <input type=\"radio\" name=\"Extra\" value=\"Blink\" checked> Blink\
  <input type=\"radio\" name=\"Extra\" value=\"Normal\" checked> Normal\
  <input type=\"radio\" name=\"Extra\" value=\"Refresh\"> Refresh\
  <input type=\"submit\" value=\"Submit\"> </p>\
</form>\
<footer>\
GVSU Senior Squad\
</footer>\
</body>\
</html>";

//------------------------------------------------------------------------------
const Timer_A_ContinuousModeConfig continuousModeConfig =
{
        TIMER_A_CLOCKSOURCE_ACLK,           // ACLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_12,      // ACLK/1 = 32.768khz
        TIMER_A_TAIE_INTERRUPT_ENABLE,      // Enable Overflow ISR
        TIMER_A_DO_CLEAR                    // Clear Counter
};
//------------------------------------------------------------------------------
const Timer_A_ContinuousModeConfig continuousModeConfigB =
{
        TIMER_A_CLOCKSOURCE_ACLK,           // ACLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_12,      // ACLK/1 = 32.768khz
        TIMER_A_TAIE_INTERRUPT_ENABLE,      // Enable Overflow ISR
        TIMER_A_DO_CLEAR                    // Clear Counter
};
//------------------------------------------------------------------------------
const eUSCI_UART_Config uartConfig =
{
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,
        6,
        8,
        0x11,
        EUSCI_A_UART_NO_PARITY,
		EUSCI_A_UART_LSB_FIRST,
		EUSCI_A_UART_ONE_STOP_BIT,
		EUSCI_A_UART_MODE,
		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
};
//------------------------------------------------------------------------------
int InitFunction_Server(void)
{
	int err = 0;

	// Turning off watch dog timer
	MAP_WDT_A_holdTimer();

	//Configuring pins for peripheral/crystal usage.
	CS_setExternalClockSourceFrequency(32768,48000000);
	MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
	MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
	MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
	CS_startHFXT(false);

	//Setting other clocks to speeds needed throughout the project
	MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
	MAP_CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);


	// Selecting P3.1 P3.2 and P3.3 in UART mode
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
			GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

	// Selecting P1.1 P1.2 and P1.3 in UART mode
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
			GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

	// Configuring SPI in 3wire master mode
	UART_initModule(EUSCI_A0_MODULE, &uartConfig);
	UART_initModule(EUSCI_A2_MODULE, &uartConfig);

	//Enable SPI module
	UART_enableModule(EUSCI_A0_MODULE);
	UART_enableModule(EUSCI_A2_MODULE);

    /* Enabling interrupts */
    MAP_UART_enableInterrupt(EUSCI_A0_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    MAP_UART_enableInterrupt(EUSCI_A2_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
    MAP_Interrupt_enableInterrupt(INT_EUSCIA2);
    MAP_Interrupt_enableSleepOnIsrExit();
    MAP_Interrupt_enableMaster();


    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);


	return err;
}
//------------------------------------------------------------------------------
int InitFunction_Client(void)
{
	int err = 0;

	RTC_C_Calendar currentTime;
	currentTime.dayOfmonth = 0x20;
	currentTime.hours = 0x18;
	currentTime.minutes = 0x00;
	currentTime.month = 0x00;
	currentTime.seconds = 0x00;
	currentTime.year = 0x2016;

	//Configuring pins for peripheral/crystal usage.
	CS_setExternalClockSourceFrequency(32000, 48000000);
	MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
	MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
	MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
	CS_startHFXT(false);

	//Setting other clocks to speeds needed throughout the project
	MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
	MAP_CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);
	MAP_CS_initClockSignal(CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);

//	/* Initializing SMCLK to HFXT (effectively 48MHz) */
//    MAP_CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
//    CS_setDCOFrequency (32000000); // set to 16 MHz (timing is set for this freq)

	/* Starting and enabling ACLK (32kHz) */
	MAP_CS_setReferenceOscillatorFrequency(CS_REFO_128KHZ);
	MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_4);
//	ACLKfreq = MAP_CS_getACLK();  // get ACLK value to verify it was set correctly

	// Selecting P3.1 P3.2 and P3.3 in UART mode
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
			GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

	// Selecting P1.1 P1.2 and P1.3 in UART mode
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
			GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

	// Configuring SPI in 3wire master mode
	UART_initModule(EUSCI_A0_MODULE, &uartConfig);
	UART_initModule(EUSCI_A2_MODULE, &uartConfig);

	//Enable SPI module
	UART_enableModule(EUSCI_A0_MODULE);
	UART_enableModule(EUSCI_A2_MODULE);

	/* Enabling interrupts */
	MAP_UART_enableInterrupt(EUSCI_A0_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	MAP_UART_enableInterrupt(EUSCI_A2_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
	MAP_Interrupt_enableInterrupt(INT_EUSCIA2);
	MAP_Interrupt_enableSleepOnIsrExit();
	MAP_Interrupt_enableMaster();

	/* Configuring Continuous Mode */
	MAP_Timer_A_configureContinuousMode(TIMER_A0_MODULE, &continuousModeConfig);
	/* Enabling interrupts and going to sleep */
	MAP_Interrupt_enableSleepOnIsrExit();
	MAP_Interrupt_enableInterrupt(INT_TA0_N);
	/* Starting the Timer_A0 in continuous mode */
	MAP_Timer_A_startCounter(TIMER_A0_MODULE, TIMER_A_CONTINUOUS_MODE);

	/* Configuring Continuous Mode */
	MAP_Timer_A_configureContinuousMode(TIMER_A1_MODULE, &continuousModeConfigB);
	/* Enabling interrupts and going to sleep */
	MAP_Interrupt_enableSleepOnIsrExit();
	MAP_Interrupt_enableInterrupt(INT_TA1_N);
	/* Starting the Timer_A0 in continuous mode */
	MAP_Timer_A_startCounter(TIMER_A1_MODULE, TIMER_A_CONTINUOUS_MODE);


	MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
	GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

	/* Initializing RTC with current time as described in time in
	 * definitions section */
	MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

	/* Specify an interrupt to assert every minute */
	MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);

	/* Enable interrupt for RTC Ready Status, which asserts when the RTC
	 * Calendar registers are ready to read.
	 * Also, enable interrupts for the Calendar alarm and Calendar event. */
	MAP_RTC_C_clearInterruptFlag(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);
	MAP_RTC_C_enableInterrupt(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);

	/* Start RTC Clock */
	MAP_RTC_C_startClock();
	return err;
}
//-----------------------------------------------------------------------
int DisplayInit (void)
{
	int err = 0;

	//initialize the screen
	Clock_Init48MHz();                   // set system clock to 48 MHz
	ST7735_InitR(INITR_GREENTAB);

	ST7735_FillScreen(0x0000);

	ST7735_SetRotation(3);
//	ST7735_DrawStringHorizontal(10, 10, "Hello World", 0x0FF0, 2);

	return err;
}
//------------------------------------------------------------------------------
//int CheckForString(char *letter)
//{
//	int found = 0;
//	static int ptr;
//
//	if(DesiredString[ptr] == *letter)
//	{
//		ptr++;
//		if(ptr == strlen(DesiredString))
//		{
//			found = 1;
//		}
//		else
//			CheckForString(letter++);
//	}
//	else
//	{
//		ptr = 0;
//		found = 0;
//	}
//
//	return found;
//}
//------------------------------------------------------------------------------
void PutInServerMode(void)
{
	__delay_cycles(1000000);
	__delay_cycles(1000000);
	__delay_cycles(1000000);
	__delay_cycles(1000000);

	sprintf(DataOUT, "AT+CIPCLOSE=5\r\n");
	ESP8266_Send(DataOUT);
	__delay_cycles(1000000);

	sprintf(DataOUT, "AT+CIPMUX=1\r\n");
	ESP8266_Send(DataOUT);
	__delay_cycles(1000000);

	sprintf(DataOUT, "AT+CIPSERVER=0\r\n");
	ESP8266_Send(DataOUT);
	__delay_cycles(1000000);

	sprintf(DataOUT, "AT+CIPSERVER=1,80\r\n");
	ESP8266_Send(DataOUT);
	__delay_cycles(1000000);
}
//------------------------------------------------------------------------------
int PutInClientMode(void)
{
	int err = 0;

	__delay_cycles(1000000);
	__delay_cycles(1000000);
	__delay_cycles(1000000);
	__delay_cycles(1000000);

	sprintf(DataOUT, "AT+CWMODE_DEF=3\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", YES);

	sprintf(DataOUT, "AT+CIPSERVER=0\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", YES);

	sprintf(DataOUT, "AT+CIPMUX=1\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", YES);


//
//	sprintf(DataOUT, "AT+CIPSERVER=1,80\r\n");
//	ESP8266_Send(DataOUT);
//	err |= WaitForResponse_String("OK", YES);

	sprintf(DataOUT, "AT+CWJAP_DEF=\"Chase's iPhone\",\"chase1994\"\r\n");
//	sprintf(DataOUT, "AT+CWJAP_DEF=\"Chase n' Co.\",\"coconutPUP\"\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("CONNECTED", YES);

	sprintf(DataOUT, "AT+CIFSR");
	ESP8266_Send(DataOUT);
//	__delay_cycles(1000000);
	err |= WaitForResponse_String("OK", YES);

	sprintf(DataOUT, "AT+CIPSERVER=1,80\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", YES);

	return err;
}
//------------------------------------------------------------------------------
int WaitForResponse_String(char *strToFind, int okToErase)
{
	int err = 0, exitLoop = 0;
	char *sp, findString[20];

	strcpy(findString, strToFind);

//	sprintf(tempstr, "Searching for string \"%s\"\n", findString);
//	my_puts(tempstr);

	//Start Timer
	Timer_A_clearTimer(TIMER_A0_MODULE);
	Timer_A_enableInterrupt(TIMER_A0_MODULE);
	TimeOutTripped = 0;

	while(!exitLoop)
	{
		if(NewData)
		{
			NewData = 0;
			sp = strstr(ESPbuffer, findString);
			if(sp != NULL)
			{
				err = 0;
				break;
			}
		}

		if(TimeOutTripped)
		{
			err = -1;
			my_puts("Timeout occured. Did not find string.\n");
			break;
		}
	}

	if(okToErase)
	{
		memset(ESPbuffer, 0, sizeof(ESPbuffer));
		bufIDx = 0;
	}

	//Turn Off Timer
	TimeOutTripped = 0;
	Timer_A_disableInterrupt(TIMER_A0_MODULE);

	return err;
}
//------------------------------------------------------------------------------
int ParseNIST_Data(int beginningByte, int endingByte, char *buf)
{
	int err = 0, count = 0;
	char temp[BUFFER_SIZE], *ptr1 = 0;
	uint32_t portID, julianTime, currentYear, currentMonth, currentDay, currentHours, currentMinutes, currentSeconds, dst;
	int monthDays[12];

	monthDays[0] = 31;	//jan
	monthDays[1] = 28;	//feb
	monthDays[2] = 31;	//mar
	monthDays[3] = 30;	//apr
	monthDays[4] = 31;	//may
	monthDays[5] = 30;	//jun
	monthDays[6] = 31;	//jul
	monthDays[7] = 31;	//aug
	monthDays[8] = 30;	//sep
	monthDays[9] = 31;	//oct
	monthDays[10] = 30;	//nov
	monthDays[11] = 31;	//dec

	memset(temp, 0, sizeof(temp));
	if(endingByte < beginningByte)
	{
		memcpy(temp, &buf[beginningByte], ((int)BUFFER_SIZE - beginningByte));
		memcpy(&temp[((int)BUFFER_SIZE - beginningByte)], &buf[0], endingByte);
	}
	else
	{
		memcpy(temp, &buf[beginningByte], (endingByte - beginningByte));
	}

	//ptr1 = strstr(temp, "local_time_rfc822");
	ptr1 = strstr(temp, "+IPD");
	if(ptr1)
	{
		count = sscanf(ptr1, "+IPD,0,%d:\n%d %d-%d-%d %d:%d:%d %d", &portID, &julianTime, &currentYear, &currentMonth, &currentDay,
				&currentHours, &currentMinutes, &currentSeconds, &dst);

		if(count < 9)
		{
			err = -1;
			TimeDate.hour = 0;
			TimeDate.minute = 0;
			TimeDate.sec = 0;
			TimeDate.month = 0;
			TimeDate.year = 0;
			TimeDate.day = 0;
		}
		else
		{
			if(currentHours >= 4)
			{
				TimeDate.hour = currentHours - 4;
				TimeDate.minute = currentMinutes;
				TimeDate.sec = currentSeconds;
				TimeDate.month = currentMonth;
				TimeDate.year = currentYear;
				TimeDate.day = currentDay;
			}
			else
			{
				TimeDate.hour = (currentHours + 24) - 4;

				if(currentDay == 1)
					if(currentMonth == 1)
					{
						TimeDate.day = (monthDays[11]);
						TimeDate.year = currentYear - 1;
						TimeDate.month = 12;
					}
					else
					{
						TimeDate.day = (monthDays[currentMonth-1]);
						TimeDate.year = currentYear;
						TimeDate.month = currentMonth;
					}
				else
				{
					TimeDate.day = currentDay - 1;
					TimeDate.year = currentYear;
					TimeDate.month = currentMonth;
				}

				TimeDate.minute = currentMinutes;
				TimeDate.sec = currentSeconds;
			}
		}
	}
	else
	{
		my_puts("An error occured in the time string.\n");
		err = -2;
	}

	return err;
}
//------------------------------------------------------------------------------
int ParseWUNDER_Data(int beginningByte, int endingByte, char *buf, int index)
{
	int err = 0, searchCounter = 0, periodCounter = 0, dataINT = 0;
	char temp[BUFFER_SIZE], *ptr1 = 0, *ptr2 = 0, *startingPoint;
	char currentWeather[50], periodString[25];

	memset(currentWeather, 0, sizeof(currentWeather));
	memset(temp, 0, sizeof(temp));
	if(endingByte < beginningByte)
	{
		memcpy(temp, &buf[beginningByte], ((int)BUFFER_SIZE - beginningByte));
		memcpy(&temp[((int)BUFFER_SIZE - beginningByte)], &buf[0], endingByte);
	}
	else
	{
		memcpy(temp, &buf[beginningByte], (endingByte - beginningByte));
	}

	if(index == 0) //Parses the forcast
	{
		for(periodCounter = 0; periodCounter < 4; periodCounter++)
		{
			memset(periodString, 0, sizeof(periodString));
			sprintf(periodString, "\"period\":%d", periodCounter+1);
			startingPoint = strstr(temp, periodString);

			if(startingPoint)
			{
				for(searchCounter = 0; searchCounter < 6; searchCounter++)
				{
					ptr1 = strstr(startingPoint, strToFind[searchCounter]);
					if(ptr1)
					{
						memset(currentWeather, 0, sizeof(currentWeather));
						if(searchCounter == 0)
						{
							ptr2 = strstr(ptr1, "fahrenheit");
							err = getDataBetweenQuotationMarks(&ptr2[12], currentWeather);
							Weather[periodCounter].fahrenheitHIGH = atoi(currentWeather);
						}
						else if(searchCounter == 1)
						{
							ptr2 = strstr(ptr1, "fahrenheit");
							err = getDataBetweenQuotationMarks(&ptr2[12], currentWeather);
							Weather[periodCounter].fahrenheitLOW = atoi(currentWeather);
						}
						else if(searchCounter == 2)
						{
							err = getDataBetweenQuotationMarks(&ptr1[spaces[searchCounter]], currentWeather);
							strcpy(Weather[periodCounter].conditions, currentWeather);
						}
						else if(searchCounter == 3)
						{
							ptr2 = strstr(ptr1, "mph");
							sscanf(ptr2, "mph\": %d,", &dataINT);
							Weather[periodCounter].avewind = dataINT;

							ptr2 = strstr(ptr1, "dir");
							err = getDataBetweenQuotationMarks(&ptr2[5], currentWeather);
							strcpy(Weather[periodCounter].windDirection, currentWeather);
						}
						else if(searchCounter == 4)
						{
							sscanf(ptr1, "\"avehumidity\": %d,", &dataINT);
							Weather[periodCounter].avehumidity = dataINT;
						}
						else if(searchCounter == 5)
						{
							sscanf(ptr1, "\"pop\":%d,", &dataINT);
							Weather[periodCounter].pop = dataINT;
						}
						else
						{
							my_puts("An error occured!!\n");
							err = -1;
						}
					}
					else
					{
						my_puts("Cannot find the time string!\n");
						err = -2;
					}
				}
			}
			else
			{
				my_puts("Cound not find starting point\n");
				err = -3;
			}
		}
	}
	else //Parses the current conditions
	{
		startingPoint = strstr(temp, "current_observation");

		if(startingPoint)
		{
			for(searchCounter = 0; searchCounter < 4; searchCounter++)
			{
				ptr1 = strstr(startingPoint, strToFind[searchCounter+6]);
				if(ptr1)
				{
					memset(currentWeather, 0, sizeof(currentWeather));
					if(searchCounter == 0)
					{
						sscanf(ptr1, "\"temp_f\":%d,", &dataINT);
						CurrentWeather.currentTemp = dataINT;
					}
					else if(searchCounter == 1)
					{
						err = getDataBetweenQuotationMarks(&ptr1[spaces[searchCounter+6]], currentWeather);
						CurrentWeather.currentHumidity = atoi(currentWeather);
					}
					else if(searchCounter == 2)
					{
						err = getDataBetweenQuotationMarks(&ptr1[spaces[searchCounter+6]], currentWeather);
						strcpy(CurrentWeather.currentCondition, currentWeather);
					}
					else if(searchCounter == 3)
					{
						sscanf(ptr1, "\"local_time_rfc822\":%s,", currentWeather);
						if(strstr(currentWeather, "Sun"))
							CurrentWeather.dayOfWeek = 0;
						else if(strstr(currentWeather, "Mon"))
							CurrentWeather.dayOfWeek = 1;
						else if(strstr(currentWeather, "Tue"))
							CurrentWeather.dayOfWeek = 2;
						else if(strstr(currentWeather, "Wed"))
							CurrentWeather.dayOfWeek = 3;
						else if(strstr(currentWeather, "Thu"))
							CurrentWeather.dayOfWeek = 4;
						else if(strstr(currentWeather, "Fri"))
							CurrentWeather.dayOfWeek = 5;
						else if(strstr(currentWeather, "Sat"))
							CurrentWeather.dayOfWeek = 6;
						else
							CurrentWeather.dayOfWeek = 0;
					}
					else if(searchCounter == 4)
					{
						my_puts("An error occured!!\n");
						err = -1;
					}
				}
				else
				{
					my_puts("Cannot find the time string!\n");
					err = -2;
				}
			}
		}
	}

	return err;
}
//------------------------------------------------------------------------------
void my_puts(char *_ptr)
{
	unsigned int i, len;

	len = strlen(_ptr);

	for(i = 0; i < len; i++)
	{
		while((UCA0IFG & UCTXIFG) != UCTXIFG);
		UCA0TXBUF = (uint8_t) _ptr[i];
	}
}
//------------------------------------------------------------------------------
void ESP8266_Send(char *_sptr)
{
	unsigned i, len;

	len = strlen(_sptr);

	for(i = 0; i < len; i++)
	{
		while((UCA2IFG & UCTXIFG) != UCTXIFG);
		UCA2TXBUF = (uint8_t) _sptr[i];
	}
}
//------------------------------------------------------------------------------
void ESP8266_Receive()
{
	//Do nothing for right now
}
//------------------------------------------------------------------------------
int GetandSetTime(void)
{
	int err = 0, startByte, endByte;
	char temp[50];
	RTC_C_Calendar setTime;

//			sprintf(DataOUT, "AT+CIPSTART=\"TCP\",\"api.wunderground.com\",80\r\n");
//			ESP8266_Send(DataOUT);
//			err |= WaitForResponse_String("OK", ptr);
//
//			sprintf(DataOUT, "AT+CIPSEND=%d\r\n", strlen(weatherString));
//			ESP8266_Send(DataOUT);
//			err |= WaitForResponse_String("OK", ptr);
//
////			memset(ESPbuffer, 0, sizeof(ESPbuffer));
//			startByte = bufIDx;
//			sprintf(DataOUT, "%s", weatherString);
//			ESP8266_Send(DataOUT);
//			err |= WaitForResponse_String("CLOSED", ptr);
//			endByte = bufIDx;

	startByte = bufIDx;
	sprintf(DataOUT, "AT+CIPSTART=0,\"TCP\",\"time.nist.gov\",13\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("CLOSED", NO);

//			sprintf(DataOUT, "AT+CIPSEND=%d\r\n", strlen(weatherString));
//			ESP8266_Send(DataOUT);
//			err |= WaitForResponse_String("OK", ptr);
//
////			memset(ESPbuffer, 0, sizeof(ESPbuffer));

//			sprintf(DataOUT, "%s", weatherString);
//			ESP8266_Send(DataOUT);
//			err |= WaitForResponse_String("CLOSED", ptr);
	endByte = bufIDx;

	err |= ParseNIST_Data(startByte, endByte, ESPbuffer);

	//Set the values (in the correct type) back into the RTC structure so that it can be updated.
	sprintf(temp, "%d, %d, %d, %d, %d, %d", TimeDate.month, TimeDate.day, TimeDate.year, TimeDate.hour, TimeDate.minute, TimeDate.sec);
	sscanf(temp, "%x, %x, %x, %x, %x, %x", &setTime.month, &setTime.dayOfmonth, &setTime.year, &setTime.hours, &setTime.minutes, &setTime.seconds);

	memset(ESPbuffer, 0, sizeof(ESPbuffer));
	bufIDx = 0;

	//Set the RTC with the values the uset chose
	MAP_RTC_C_initCalendar(&setTime, RTC_C_FORMAT_BCD);
	MAP_RTC_C_startClock();

	return err;
}
//------------------------------------------------------------------------------
int GetWUNDERdata(int index)
{
	int err = 0, startByte, endByte;
	char *ptr = 0;

	if(index)
	{
		sprintf(DataOUT, "AT+CIPSTART=1,\"TCP\",\"api.wunderground.com\",80\r\n");
		ESP8266_Send(DataOUT);
		err |= WaitForResponse_String("OK", YES);

		memset(ESPbuffer, 0, sizeof(ESPbuffer));
		bufIDx = 0;
		startByte = bufIDx;

		sprintf(DataOUT, "AT+CIPSEND=1,%d\r\n", strlen(weatherString));
		ESP8266_Send(DataOUT);
		err |= WaitForResponse_String("OK", YES);

		sprintf(DataOUT, "%s", weatherString);
		ESP8266_Send(DataOUT);
		err |= WaitForResponse_String("CLOSED", NO);
		endByte = bufIDx;

	}
	else
	{
		sprintf(DataOUT, "AT+CIPSTART=1,\"TCP\",\"api.wunderground.com\",80\r\n");
		ESP8266_Send(DataOUT);
		err |= WaitForResponse_String("OK", YES);

		memset(ESPbuffer, 0, sizeof(ESPbuffer));
		bufIDx = 0;
		startByte = bufIDx;

		sprintf(DataOUT, "AT+CIPSEND=1,%d\r\n", strlen(forecastString));
		ESP8266_Send(DataOUT);
		err |= WaitForResponse_String("OK", YES);

		sprintf(DataOUT, "%s", forecastString);
		ESP8266_Send(DataOUT);
		err |= WaitForResponse_String("CLOSED", NO);
		endByte = bufIDx;

		ptr = strstr(ESPbuffer, "simpleforecast");
		if(ptr)
		{
			startByte = strlen(ESPbuffer) - strlen(ptr);
		}
	}

	err |= ParseWUNDER_Data(startByte, endByte, ESPbuffer, index);

	memset(ESPbuffer, 0, sizeof(ESPbuffer));
	bufIDx = 0;

	return err;
}
//------------------------------------------------------------------------------
int getDataBetweenQuotationMarks(char *ptr, char dataReturned[80])
{
	int err = 0, i = 0, j = 0;
	int foundQuote = 0;

	while(j < 80)
	{
		if(ptr[i] == '"')
		{
			if(foundQuote)
			{
				foundQuote = 2;
				break;
			}
			else
			{
				foundQuote = 1;
				i++;
			}
		}

		if(foundQuote)
			dataReturned[j++] = ptr[i];

		i++;
	}

	if(j > 80)
		err = -2;

	if(foundQuote != 2)
		err = -3;

	return err;
}
//------------------------------------------------------------------------------
int BallColorDecision(void)
{
	int err = 0, dif = 0;

	dif = Weather[1].fahrenheitHIGH - Weather[0].fahrenheitHIGH;

	if(dif > 3)
	{
		BallColorMASTER = RED;
		BallColor = RED;
	}
	else if (dif < -3)
	{
		BallColorMASTER = BLUE;
		BallColor = BLUE;
	}
	else
	{
		BallColorMASTER = GREEN;
		BallColor = GREEN;
	}

	if(strstr(Weather[1].conditions, "Rain") || strstr(Weather[1].conditions, "Snow"))
	{
		BallBlink = YES;
	}
	else
	{
		BallBlink = NO;
	}

	return err;
}
//------------------------------------------------------------------------------
int LaunchWebsite(int channel)
{
	int err = 0;
	char tempString[300];

	sprintf(tempString, "<br>--Today's Weather--<br>\
			Conditition: %s<br>\
			High: %d F<br>\
			Low: %d F<br>\
			", Weather[0].conditions, Weather[0].fahrenheitHIGH, Weather[0].fahrenheitLOW);

	sprintf(DataOUT, "AT+CIPSEND=%d,%d\r\n", channel, strlen(httpStript1)+strlen(httpStript2)+strlen(tempString)+2);
	ESP8266_Send(DataOUT);
	WaitForResponse_String(">", NO);

	__delay_cycles(10000000);
	sprintf(DataOUT, "%s%s%s\r\n\r\n", httpStript1, tempString, httpStript2);
	ESP8266_Send(DataOUT);
	__delay_cycles(10000000);
	WaitForResponse_String("OK", YES);

	__delay_cycles(10000000);
	sprintf(DataOUT, "AT+CIPCLOSE=%d\r\n", channel);
	ESP8266_Send(DataOUT);
	WaitForResponse_String("CLOSED", YES);

	return err;
}
//------------------------------------------------------------------------------
int NextDateString(int currentDay, char *dayStringReturn, int daysOut)
{
	int err = 0;
	char dayStrings[7][12];

	memset(dayStrings, 0, sizeof(dayStrings));

	strcpy(dayStrings[0], "Sunday");
	strcpy(dayStrings[1], "Monday");
	strcpy(dayStrings[2], "Tuesday");
	strcpy(dayStrings[3], "Thursday");
	strcpy(dayStrings[4], "Friday");
	strcpy(dayStrings[5], "Saturday");
	strcpy(dayStrings[6], "Sunday");

	if((currentDay+daysOut > 6))
	{
		strcpy(dayStringReturn, dayStrings[(currentDay+daysOut)-6]);
//		dayStringReturn[strlen(dayStrings(currentDay+daysOut)-6)] = 0;
	}
	else
	{
		strcpy(dayStringReturn, dayStrings[(currentDay+daysOut)]);
//		dayStringReturn[dayStrings(currentDay+daysOut)] = 0;
	}



	return err;
}
