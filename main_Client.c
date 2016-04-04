#include <driverlib.h>
#include <msp432.h>
#include <string.h>
#include <stdio.h>
#include "stdint.h"

#define MAIN_PROGRAM 1

#include "support.h"
char weatherString[] = "GET http://api.wunderground.com/api/6515cde8f15218cd/conditions/q/MI/Grand_Rapids.json HTTP/1.1\r\nHost:api.wunderground.com\r\nConnection:close\r\n\r\n";



//------------------------------------------------------------------------------
void main(void)
{
	int err = 0, startByte, endByte;
	char *ptr, tempStr[] = "\"observation_epoch\":\"1459584977\",\
		\"local_time_rfc822\":\"Sat, 02 Apr 2016 04:16:21 -0400\",\
		\"local_epoch\":\"1459584981\",\
		\"local_tz_short\":\"EDT\",\
		\"local_tz_long\":\"America/New_York\",\
		\"local_tz_offset\":\"-0400\",\
		\"weather\":\"Overcast\",\
";
	char timeString[50];

	Refresh = 0;
	bufIDx = 0;
	RTC_tick = 0;

//	err = ParseData(0, strlen(tempStr),tempStr);
//
//	while(1)
//	{
//
//	}



	err = InitFunction_Client();
	if(err)
	{
		my_puts("Initialization Failed!!!\n");
	}
	else
		my_puts("Initialization PASSED!!!\n");

	MAP_Interrupt_enableMaster();
	Timer_A_disableInterrupt(TIMER_A1_MODULE);

	SMfreq = MAP_CS_getSMCLK();  // get ACLK value to verify it was set correctly
	MCLKfreq = MAP_CS_getMCLK();  // get ACLK value to verify it was set correctly
	ACLKfreq = MAP_CS_getACLK();  // get ACLK value to verify it was set correctly

	PutInClientMode();
	Timer_A_enableInterrupt(TIMER_A1_MODULE);

	err = GetandSetTime();

	/* Enable interrupts and go to sleep. */
	MAP_Interrupt_enableInterrupt(INT_RTC_C);
	MAP_Interrupt_enableSleepOnIsrExit();

	while(1)
	{
		while(!RTC_tick)
		{
		}
		RTC_tick = 0;

		sprintf(timeString,"%02x:%02x:%02x     %02x/%02x/%02x\n", newTime.hours, newTime.minutes, newTime.seconds, newTime.month, newTime.dayOfmonth, newTime.year);
		my_puts(timeString);
	}
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void euscia0_isr(void)
{
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

	MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, status);

	if(status & EUSCI_A_UART_RECEIVE_INTERRUPT)
	{
		U0RXData = MAP_UART_receiveData(EUSCI_A0_BASE);
		MAP_UART_transmitData(EUSCI_A2_BASE, U0RXData);
		MAP_UART_transmitData(EUSCI_A0_BASE, U0RXData);

		GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
		GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
	}
}
//------------------------------------------------------------------------------
void euscia2_isr(void)
{
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);
	static int bufIDx_LAST = 0;

	MAP_UART_clearInterruptFlag(EUSCI_A2_BASE, status);

	if(status & EUSCI_A_UART_RECEIVE_INTERRUPT)
	{
		GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
		GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);

		U2RXData = MAP_UART_receiveData(EUSCI_A2_BASE);
		if(U2RXData == '\n')
		{
			NewData = 1;
			memset(tempBuf, 0, sizeof(tempBuf));
			if(bufIDx < bufIDx_LAST)
			{
				memcpy(tempBuf, &ESPbuffer[bufIDx_LAST], ((int)BUFFER_SIZE - bufIDx_LAST));
				memcpy(&tempBuf[((int)BUFFER_SIZE - bufIDx_LAST)], &ESPbuffer[0], bufIDx);
			}
			else
				memcpy(tempBuf, &ESPbuffer[bufIDx_LAST], (bufIDx - bufIDx_LAST));

			bufIDx_LAST = bufIDx;
		}
//		if(!strlen(ESPbuffer))
//		{
//			bufIDx = 0;
//			bufIDx_LAST = 0;
//		}

		if(bufIDx > BUFFER_SIZE)
		{
			bufIDx = 0;
		}
		ESPbuffer[bufIDx++] = U2RXData;

		while(!(UCA0IFG & UCTXIFG));
		UCA0TXBUF = U2RXData;
	}
}
//--------------------------------------------------------------------------
void timer0_a0_isr(void)
{
	MAP_Timer_A_clearInterruptFlag(TIMER_A0_MODULE);
	TimeOutTripped = 1;
}
//--------------------------------------------------------------------------
void timer0_a1_isr(void)
{
	MAP_Timer_A_clearInterruptFlag(TIMER_A1_MODULE);
	Refresh = 1;
}
//-----------------------------------------------------------------------
/* RTC ISR */
void rtc_isr(void)
{
    uint32_t status;

    //interupt hits every second
    RTC_tick = 1;
    status = MAP_RTC_C_getEnabledInterruptStatus();
    MAP_RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_CLOCK_READ_READY_INTERRUPT)
    {
    	//Toggle the LED of launchpad, get new time from RTC
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        newTime = MAP_RTC_C_getCalendarTime();
    }
}
