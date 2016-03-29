#include <driverlib.h>
#include <msp432.h>
#include <string.h>
#include "stdint.h"

#define	IDLE			0
#define	GET_IPD			1
#define	LAUNCH_WEBSITE	2
#define	GET_DATA		3





int InitFunction(void);
void my_puts(char *_ptr);
char DesiredString[100];
int CheckForString(char *letter);


char ESPbuffer[1024], DesiredStrings[2][100], DataOUT[2038];
uint16_t U0RXData, U2RXData;
uint32_t SMfreq, MCLKfreq;
int bufIDx, RXFlag = 0, CurrentState = 0;
const char httpStript[] = "<form action=\"action_page.php\">\
  <form action=\"\"> \
  <input type=\"radio\" name=\"LED\" value=\"Red\"> Red<br> \
  <input type=\"radio\" name=\"LED\" value=\"Blue\"> Blue<br> \
  <input type=\"radio\" name=\"LED\" value=\"Green\"> Green \
  <br> \
  <input type=\"submit\" value=\"Submit\"> \
  </form>";



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
void main(void)
{
	int err = 0, i;
	char tempBuf[1024], *ptr, *ptr1, tempChar, tempParseBuffer[100];
	static int bufIDx_LAST = 0, selectChannel[5];
	memset(DesiredStrings, 0, sizeof(DesiredStrings));
	sprintf(DesiredStrings[0], "GET");
	sprintf(DesiredStrings[1], "+IPD");


	bufIDx = 0;

	err = InitFunction();
	if(err)
	{
		my_puts("Initialization Failed!!!\n");
	}
	else
		my_puts("Initialization PASSED!!!\n");

	MAP_Interrupt_enableMaster();

	SMfreq = MAP_CS_getSMCLK();  // get ACLK value to verify it was set correctly
	MCLKfreq = MAP_CS_getMCLK();  // get ACLK value to verify it was set correctly

	PutInServerMode();

	while(1)
	{
		switch(CurrentState)
		{
		case IDLE:
			break;
		case GET_IPD:
			CurrentState = LAUNCH_WEBSITE;

			memset(tempParseBuffer, 0, sizeof(tempParseBuffer));
			memcpy(tempParseBuffer, ptr, 7);
			sscanf(tempParseBuffer, "+IPD,%d,", &selectChannel[0]);
			break;
		case LAUNCH_WEBSITE:
			CurrentState = GET_DATA;
			sprintf(DataOUT, "AT+CIPSEND=%d,%d\r\n", selectChannel[0], strlen(httpStript)+2);
			ESP8266_Send(DataOUT);
			sprintf(DataOUT, "%s\r\n\r\n", httpStript);
			ESP8266_Send(DataOUT);
			__delay_cycles(1000000);
			sprintf(DataOUT, "AT+CIPCLOSE=%d\r\n", selectChannel[0]);
			ESP8266_Send(DataOUT);
//			sprintf(DataOUT, "AT+CIPSEND=+++\r\n");
//			ESP8266_Send(DataOUT);
			break;
		case GET_DATA:
			break;
		}



		if(RXFlag)
		{
			RXFlag = 0;
			memset(tempBuf, 0, sizeof(tempBuf));
			if(bufIDx < bufIDx_LAST)
			{
				memcpy(tempBuf, &ESPbuffer[bufIDx_LAST], (1024 - bufIDx_LAST));
				memcpy(&tempBuf[(1024 - bufIDx_LAST)], &ESPbuffer[0], bufIDx);
			}
			else
				memcpy(tempBuf, &ESPbuffer[bufIDx_LAST], (bufIDx - bufIDx_LAST));

			bufIDx_LAST = bufIDx;

			for(i = 0; i < 2; i++)
			{
				ptr = strstr(tempBuf, DesiredStrings[i]);
				if(ptr)
				{
					if(i == 0)
					{
						ptr1 = strstr(ptr, "LED=");
						if(ptr1)
						{
							tempChar = ptr1[4];
						}

						if(tempChar == 'B')
						{
							GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
							GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);	//Turn blue
						}
						else if(tempChar == 'G')
						{
							GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
							GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);//Turn Green
						}
						else if(tempChar == 'R')
						{
							GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
							GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);//Turn Red
						}
					}
					else if(i == 1)
					{
						CurrentState = GET_IPD;
					}
					else if(i == 2)
					{

					}
				}
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
			RXFlag = 1;
		if(bufIDx > 1023)
		{
			bufIDx = 0;
		}
		ESPbuffer[bufIDx++] = U2RXData;
		while(!(UCA0IFG & UCTXIFG));
		UCA0TXBUF = U2RXData;
	}
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
int InitFunction(void)
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
int CheckForString(char *letter)
{
	int found = 0;
	static int ptr;

	if(DesiredString[ptr] == *letter)
	{
		ptr++;
		if(ptr == strlen(DesiredString))
		{
			found = 1;
		}
		else
			CheckForString(letter++);
	}
	else
	{
		ptr = 0;
		found = 0;
	}

	return found;
}
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
