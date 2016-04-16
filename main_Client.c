#include <driverlib.h>
#include <msp432.h>
#include <string.h>
#include <stdio.h>
#include "stdint.h"

#define MAIN_PROGRAM 1

#include "support.h"
#include "ST7735.h"
#include "ClockSystem.h"
#include "Light.h"

//------------------------------------------------------------------------------
void main(void)
{
	int err = 0, j = 0, numLEDs = 7, i, marker = 0;
	static int temp = 0;
	int tempLightState, tempColor = 0, HTMLcolor = 0;
	char timeString[50], weatherString[125], *ptr, *ptr1, tempStr[50];

	// Turning off watch dog timer
	MAP_WDT_A_holdTimer();

	strcpy(strToFind[0], "\"high\"");
	spaces[0] = strlen(strToFind[0]);
	strcpy(strToFind[1], "\"low\"");
	spaces[1] = strlen(strToFind[1]);
	strcpy(strToFind[2], "conditions\"");
	spaces[2] = strlen(strToFind[2]);
	strcpy(strToFind[3], "\"avewind\"");
	spaces[3] = strlen(strToFind[3]);
	strcpy(strToFind[4], "\"avehumidity\"");
	spaces[4] = strlen(strToFind[4]);

	Refresh = 0;
	bufIDx = 0;
	RTC_tick = 1;

	err = DisplayInit();

	err = -1;
	while(err)
	{
		err = InitFunction_Client();
		if(err)
		{
			my_puts("Initialization Failed!!!\n");
		}
		else
			my_puts("Initialization PASSED!!!\n");
	}

	MAP_Interrupt_enableMaster();
	Timer_A_disableInterrupt(TIMER_A1_MODULE);

	SMfreq = MAP_CS_getSMCLK();  // get ACLK value to verify it was set correctly
	MCLKfreq = MAP_CS_getMCLK();  // get ACLK value to verify it was set correctly
	ACLKfreq = MAP_CS_getACLK();  // get ACLK value to verify it was set correctly

	err = -1;
	while(err)
	{
		err = PutInClientMode();
	}
	Timer_A_enableInterrupt(TIMER_A1_MODULE);

	err = -1;
	while(err)
	{
		err = GetandSetTime();
		__delay_cycles(10000000);
		newTime = MAP_RTC_C_getCalendarTime();
		RTC_tick = 1;
	}

	err = InitLight();

	/* Enable interrupts and go to sleep. */
	MAP_Interrupt_enableInterrupt(INT_RTC_C);
	MAP_Interrupt_enableSleepOnIsrExit();

	err = -1;
	while(err)
	{
		err = GetWUNDERdata(0);
		if(err)
			__delay_cycles(10000000);
	}

	BallColorDecision();

	for(j = 0; j < 4; j++)
	{
		sprintf(weatherString,"High Temp: %d\nLow Temp: %d\nCondition: %s\nWind Speed: %d \nWind Direction: %s\nHumidity: %d \n\n", Weather[j].fahrenheitHIGH,
				Weather[j].fahrenheitLOW, Weather[j].conditions, Weather[j].avewind, Weather[j].windDirection, Weather[j].avehumidity);
		my_puts(weatherString);

		__delay_cycles(10000000);
	}

	One_Day = 1;
	while(1)
	{
		if(WeatherUpdate)
		{
			WeatherUpdate = 0;
			err = -1;
			while(err)
			{
				err = GetWUNDERdata(0);
				BallColorDecision();
				if(err)
					__delay_cycles(10000000);
			}
		}

		if(RTC_tick)
		{
			sprintf(timeString,"%02x:%02x:%02x     %02x/%02x/%02x\n", newTime.hours, newTime.minutes, newTime.seconds, newTime.month, newTime.dayOfmonth, newTime.year);
			my_puts(timeString);
		}

		if(temp++ > 150000)
		{
			temp = 0;;

			if(BallColor == RED)
				SpinningColor(0x0F, 0x00, 0x00, 0x00);
			else if(BallColor == BLUE)
				SpinningColor(0x00, 0x00, 0x0F, 0x00);
			else if(BallColor == GREEN)
				SpinningColor(0x00, 0x0F, 0x00, 0x00);
			else
				SpinningColor(0x00, 0x00, 0x00, 0x00);
		}

//		//Update the LEDs
//		if(LightUpdate || HTML_LightUpdate)
//		{
//			//Control the lights here
//			LightUpdate = 0;
//
//			if(HTML_LightUpdate)
//			{
//				tempColor = BallColor;
//				BallColor = HTMLcolor;
//			}
//
//			if(BallBlink && !HTML_LightUpdate)
//			{
//				if(tempLightState)
//					tempLightState = 0;
//				else
//					tempLightState = 1;
//			}
//			else
//				tempLightState = 1;
//
//			if(BallColor == RED && tempLightState)
//				gradualFill(numLEDs, 0x0F, 0x00, 0x00, 0x00);
//			else if(BallColor == BLUE && tempLightState)
//				gradualFill(numLEDs, 0x00, 0x00, 0x0F, 0x00);
//			else if(BallColor == GREEN && tempLightState)
//				gradualFill(numLEDs, 0x00, 0x0F, 0x00, 0x00);
//			else
//				gradualFill(numLEDs, 0x00, 0x00, 0x00, 0x00);
//
//			if(HTML_LightUpdate)
//			{
//				HTML_LightUpdate = 0;
//				BallColor = tempColor;
//			}
//		}

		//Parse data that will be coming through from the HTML wab page
		if(NewData)
		{
			NewData = 0;
			ptr = strstr(ESPbuffer, "+IPD");
			if(ptr)
			{
				sscanf(ptr, "+IPD,%d,", &IPDChannel);
				WaitForResponse_String("Connection", NO);

				ptr = strstr(ESPbuffer, "action_page.php");
				if(!ptr)
				{
					LaunchWebsite(IPDChannel);
				}
				else
				{
					ptr1 = strstr(ptr, "action_page.php?LED=");
					if(ptr1)
					{
						LightTimer = 0;
						sscanf(ptr, "action_page.php?LED=%s HTTP", tempStr);
						if(strstr(tempStr, "Red"))
							BallColor = RED;
						else if(strstr(tempStr, "Blue"))
							BallColor = BLUE;
						else if(strstr(tempStr, "Green"))
							BallColor = GREEN;

						HTML_LightUpdate = 1;
					}

					ptr1 = strstr(ptr, "action_page.php?Screen=");
					if(ptr1)
					{
						sscanf(ptr, "action_page.php?Screen=%s HTTP", tempStr);
						if(strstr(tempStr, "Today"))
							One_Day = 1;
						else if(strstr(tempStr, "Tomorrow"))
							Two_Day = 1;
						else if(strstr(tempStr, "Scrolling"))
							Five_Day = 1;
					}

					ptr1 = strstr(ptr, "action_page.php?Extra=");
					if(ptr1)
					{
						sscanf(ptr, "action_page.php?Extra=%s HTTP", tempStr);
						if(strstr(tempStr, "Blink"))
						{
							BallBlink = 1;
							LightTimer = 0;
						}
						else if(strstr(tempStr, "Normal"))
						{
							BallBlink = 0;
							LightTimer = 0;
						}
						else if(strstr(tempStr, "Refresh"))
							WeatherUpdate = 1;
					}

					WaitForResponse_String(" ", YES);
				}
			}
		}

		if(One_Day)
		{
			One_Day = 0; Two_Day = 0; Five_Day = 0;
			ST7735_FillScreen(0x0000);

			sprintf(tempStr, "%s", Weather[0].conditions);
			ST7735_DrawStringHorizontal(0, 18, tempStr, 0x0FF0, 2);
			sprintf(tempStr, "High Temp: %d", Weather[0].fahrenheitHIGH);
			ST7735_DrawStringHorizontal(0, 45, tempStr, 0x0FF0, 2);
			sprintf(tempStr, "Low Temp: %d", Weather[0].fahrenheitLOW);
			ST7735_DrawStringHorizontal(60, 62, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "Humidity: %d", Weather[0].avehumidity);
			ST7735_DrawStringHorizontal(0, 80, tempStr, 0x0FF0, 2);
			sprintf(tempStr, "Wind:%dmph %s", Weather[0].avewind, Weather[0].windDirection);
			ST7735_DrawStringHorizontal(0, 110, tempStr, 0x0FF0, 2);
		}

		if(Two_Day)
		{
			One_Day = 0; Two_Day = 0; Five_Day = 0;
			ST7735_FillScreen(0x0000);
			ST7735_DrawStringVertical(72, 0, "|||||||||||||||", 0x0FF0, 1);

			sprintf(tempStr, "-Today-");
			ST7735_DrawStringHorizontal(0, 11, tempStr, 0x00FF, 1);
			sprintf(tempStr, "%s", Weather[0].conditions);
			ST7735_DrawStringHorizontal(0, 27, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "Temp H: %d", Weather[0].fahrenheitHIGH);
			ST7735_DrawStringHorizontal(0, 45, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "Temp L: %d", Weather[0].fahrenheitLOW);
			ST7735_DrawStringHorizontal(0, 62, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "Humidity: %d", Weather[0].avehumidity);
			ST7735_DrawStringHorizontal(0, 80, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "W: %dmph %s", Weather[0].avewind, Weather[0].windDirection);
			ST7735_DrawStringHorizontal(0, 110, tempStr, 0x0FF0, 1);

			sprintf(tempStr, "-Tomorrow-");
			ST7735_DrawStringHorizontal(78, 11, tempStr, 0x00FF, 1);
			sprintf(tempStr, "%s", Weather[1].conditions);
			ST7735_DrawStringHorizontal(78, 27, tempStr, 0xFF00, 1);
			sprintf(tempStr, "Temp H: %d", Weather[1].fahrenheitHIGH);
			ST7735_DrawStringHorizontal(78, 45, tempStr, 0xFF00, 1);
			sprintf(tempStr, "Temp L: %d", Weather[1].fahrenheitLOW);
			ST7735_DrawStringHorizontal(78, 62, tempStr, 0xFF00, 1);
			sprintf(tempStr, "Humidity: %d", Weather[1].avehumidity);
			ST7735_DrawStringHorizontal(78, 80, tempStr, 0xFF00, 1);
			sprintf(tempStr, "W: %dmph %s", Weather[1].avewind, Weather[1].windDirection);
			ST7735_DrawStringHorizontal(78, 110, tempStr, 0xFF00, 1);
		}

		if(Five_Day && ScrollingTick)
		{
			RTC_tick = 1;
			One_Day = 0; Two_Day = 0;
			ScrollingTick = 0;

			ST7735_FillScreen(0x0000);
			ST7735_DrawStringVertical(72, 0, "|||||||||||||||", 0x0FF0, 1);

			sprintf(tempStr, "Day: %d", marker);
			ST7735_DrawStringHorizontal(0, 11, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "%s", Weather[marker].conditions);
			ST7735_DrawStringHorizontal(0, 27, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "Temp H: %d", Weather[marker].fahrenheitHIGH);
			ST7735_DrawStringHorizontal(0, 45, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "Temp L: %d", Weather[marker].fahrenheitLOW);
			ST7735_DrawStringHorizontal(0, 62, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "Humidity: %d", Weather[marker].avehumidity);
			ST7735_DrawStringHorizontal(0, 80, tempStr, 0x0FF0, 1);
			sprintf(tempStr, "W: %dmph %s", Weather[marker].avewind, Weather[marker].windDirection);
			ST7735_DrawStringHorizontal(0, 110, tempStr, 0x0FF0, 1);

			sprintf(tempStr, "Day: %d", marker + 1);
			ST7735_DrawStringHorizontal(78, 11, tempStr, 0xFF00, 1);
			sprintf(tempStr, "%s", Weather[marker + 1].conditions);
			ST7735_DrawStringHorizontal(78, 27, tempStr, 0xFF00, 1);
			sprintf(tempStr, "Temp H: %d", Weather[marker + 1].fahrenheitHIGH);
			ST7735_DrawStringHorizontal(78, 45, tempStr, 0xFF00, 1);
			sprintf(tempStr, "Temp L: %d", Weather[marker + 1].fahrenheitLOW);
			ST7735_DrawStringHorizontal(78, 62, tempStr, 0xFF00, 1);
			sprintf(tempStr, "Humidity: %d", Weather[marker + 1].avehumidity);
			ST7735_DrawStringHorizontal(78, 80, tempStr, 0xFF00, 1);
			sprintf(tempStr, "W: %dmph %s", Weather[marker + 1].avewind, Weather[marker + 1].windDirection);
			ST7735_DrawStringHorizontal(78, 110, tempStr, 0xFF00, 1);

			if(++marker > 2)
				marker = 0;
		}

		if(RTC_tick)
		{
			RTC_tick = 0;
			//Create the time string that will be written
			sprintf(timeString, "     %02X:%02X    %02X/%02X/%X       ", newTime.hours, newTime.minutes, newTime.month, newTime.dayOfmonth, newTime.year);

			//Write the new time string
			for(i = 0; i < strlen(timeString); i++)
			{
				ST7735_DrawChar((6*i), 0, timeString[i], ST7735_Color565(255, 255, 255), ST7735_Color565(255, 0, 0), 1);
			}
		}
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
	}
}
//------------------------------------------------------------------------------
void euscia2_isr(void)
{
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

	MAP_UART_clearInterruptFlag(EUSCI_A2_BASE, status);

	if(status & EUSCI_A_UART_RECEIVE_INTERRUPT)
	{
		U2RXData = MAP_UART_receiveData(EUSCI_A2_BASE);
		if(U2RXData == '\n')
		{
			NewData = 1;
		}

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
	static int j, i;
    uint32_t status;

    if(j++ > 6)
    {
    	j = 0;
    	ScrollingTick = 1;
    }


    if(LightTimer++ > 60)
    {
    	LightTimer = 0;
    	LightUpdate = 1;
    	BallColor = BallColorMASTER;
    	BallBlink = BallBlinkMASTER;
    }

    status = MAP_RTC_C_getEnabledInterruptStatus();
    MAP_RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_TIME_EVENT_INTERRUPT)
    {
    	//Toggle the LED of launchpad, get new time from RTC
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        newTime = MAP_RTC_C_getCalendarTime();
        RTC_tick = 1;

        if(i++ > 15)
        {
        	i = 0;
        	WeatherUpdate = 1;
        }
    }
}
//-----------------------------------------------------------------------
void SysTick_ISR(void)
{
	ms_timeout=1;  // set flag for ms timer
	ms10Timer++;  // increment on each interrupt to count up to 10 milliseconds
    if(ms10Timer>10)
    {
    	ms10Timer=0;
    }
}
