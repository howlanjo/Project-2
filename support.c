#include <driverlib.h>
#include <msp432.h>
#include <string.h>
#include <stdio.h>
#include "stdint.h"
#include "support.h"

//------------------------------------------------------------------------------
const Timer_A_ContinuousModeConfig continuousModeConfig =
{
        TIMER_A_CLOCKSOURCE_ACLK,           // ACLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_6,      // ACLK/1 = 32.768khz
        TIMER_A_TAIE_INTERRUPT_ENABLE,      // Enable Overflow ISR
        TIMER_A_DO_CLEAR                    // Clear Counter
};
//------------------------------------------------------------------------------
const Timer_A_ContinuousModeConfig continuousModeConfigB =
{
        TIMER_A_CLOCKSOURCE_ACLK,           // ACLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_8,      // ACLK/1 = 32.768khz
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
	MAP_CS_initClockSignal(CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);

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
void PutInClientMode(void)
{
	int err = 0;
	char *ptr;
	__delay_cycles(1000000);
	__delay_cycles(1000000);
	__delay_cycles(1000000);
	__delay_cycles(1000000);

	sprintf(DataOUT, "AT+CWMODE_DEF=3\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", ptr);

	sprintf(DataOUT, "AT+CWJAP_DEF=\"Chase's iPhone\",\"chase1994\"\r\n");
//	sprintf(DataOUT, "AT+CWJAP_DEF=\"Chase n' Co.\",\"coconutPUP\"\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("CONNECTED", ptr);

	sprintf(DataOUT, "AT+CIFSR");
	ESP8266_Send(DataOUT);
//	__delay_cycles(1000000);
	err |= WaitForResponse_String("OK", ptr);

	sprintf(DataOUT, "AT+CIPSERVER=0\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", ptr);

	sprintf(DataOUT, "AT+CIPSERVER=1,80\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", ptr);

	sprintf(DataOUT, "AT+CIPMUX=0\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("OK", ptr);
}
//------------------------------------------------------------------------------
int WaitForResponse_String(char *strToFind, char *returnPtr)
{
	int err = 0, exitLoop = 0;
	char *sp, *str, findString[20], tempstr[50];

	strcpy(findString, strToFind);

	sprintf(tempstr, "Searching for string \"%s\"\n", findString);
	my_puts(tempstr);

	//Start Timer
	Timer_A_clearTimer(TIMER_A0_MODULE);
	Timer_A_enableInterrupt(TIMER_A0_MODULE);
	TimeOutTripped = 0;

	while(!exitLoop)
	{
		if(strlen(tempBuf) && NewData)
		{
//			printf("Found Data.\n");
			NewData = 0;
			sp = strstr(tempBuf, findString);
			if(sp != NULL)
			{
				sprintf(tempstr, "Found \"%s\"!!\n", findString);
				my_puts(tempstr);
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

	returnPtr = sp;
	//Turn Off Timer
	TimeOutTripped = 0;
	Timer_A_disableInterrupt(TIMER_A0_MODULE);

	return err;
}
//------------------------------------------------------------------------------
int ParseData(int beginningByte, int endingByte, char *buf)
{
	int err = 0, count = 0;
	char temp[BUFFER_SIZE], *ptr1 = 0, *ptr2 = 0, *ptr3 = 0, str[200];
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
		count = sscanf(ptr1, "+IPD,%d:\n%d %d-%d-%d %d:%d:%d %d", &portID, &julianTime, &currentYear, &currentMonth, &currentDay,
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
					TimeDate.day = currentDay - 1;

				TimeDate.minute = currentMinutes;
				TimeDate.sec = currentSeconds;
			}
		}
	}
	else
	{
		my_puts("Cannot find the time string!\n");
		err = -1;
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
	char *ptr, temp[50];
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
	sprintf(DataOUT, "AT+CIPSTART=\"TCP\",\"time.nist.gov\",13\r\n");
	ESP8266_Send(DataOUT);
	err |= WaitForResponse_String("CLOSED", ptr);

//			sprintf(DataOUT, "AT+CIPSEND=%d\r\n", strlen(weatherString));
//			ESP8266_Send(DataOUT);
//			err |= WaitForResponse_String("OK", ptr);
//
////			memset(ESPbuffer, 0, sizeof(ESPbuffer));

//			sprintf(DataOUT, "%s", weatherString);
//			ESP8266_Send(DataOUT);
//			err |= WaitForResponse_String("CLOSED", ptr);
	endByte = bufIDx;

	err |= ParseData(startByte, endByte, ESPbuffer);

	//Set the values (in the correct type) back into the RTC structure so that it can be updated.
	sprintf(temp, "%d, %d, %d, %d, %d, %d", TimeDate.month, TimeDate.day, TimeDate.year, TimeDate.hour, TimeDate.minute, TimeDate.sec);
	sscanf(temp, "%x, %x, %x, %x, %x, %x", &setTime.month, &setTime.dayOfmonth, &setTime.year, &setTime.hours, &setTime.minutes, &setTime.seconds);

	//Set the RTC with the values the uset chose
	MAP_RTC_C_initCalendar(&setTime, RTC_C_FORMAT_BCD);
	MAP_RTC_C_startClock();

	return err;
}
