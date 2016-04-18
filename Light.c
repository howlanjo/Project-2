#include "driverlib.h"
#include <msp432.h>
#include <string.h>
#include <stdio.h>
#include "stdint.h"

#include "Light.h"
#include "support.h"

/* SPI Master Configuration Parameter */
const eUSCI_SPI_MasterConfig LIGHTspiConfig =
{
        EUSCI_B_SPI_CLOCKSOURCE_SMCLK,             // SMCLK Clock Source
        48000000,                                   // SMCLK 32MHz
        16000000,                                    // SPICLK = 16MHz
        EUSCI_B_SPI_MSB_FIRST,                     // MSB First
        EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT,    // Phase
        EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH, // High polarity
        EUSCI_B_SPI_3PIN                           // 3Wire SPI Mode
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int spi_Open(void)
{
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6,
			GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring SPI in 3wire master mode */
	SPI_initMaster(EUSCI_B1_MODULE, &LIGHTspiConfig);

	/* Enable SPI module */
	SPI_enableModule(EUSCI_B1_MODULE);

    return 0;//NONOS_RET_OK;
}
//------------------------------------------------------------------------------
// Initializes everything needed to use this library. This clears the strip.
void initStrip()
{
// initialize eUSCI
	UCB1CTLW0 = 0x0001;                   // hold the eUSCI module in reset mode
	  // configure UCA3CTLW0 for:
	  // bit15      UCCKPH = 1; data shifts in on first edge, out on following edge
	  // bit14      UCCKPL = 0; clock is low when inactive
	  // bit13      UCMSB = 1; MSB first
	  // bit12      UC7BIT = 0; 8-bit data
	  // bit11      UCMST = 1; master mode
	  // bits10-9   UCMODEx = 2; UCSTE active low
	  // bit8       UCSYNC = 1; synchronous mode
	  // bits7-6    UCSSELx = 2; eUSCI clock SMCLK
	  // bits5-2    reserved
	  // bit1       UCSTEM = 1; UCSTE pin enables slave
	  // bit0       UCSWRST = 1; reset enabled
	UCB1CTLW0 = 0xAD83;
	  // set the baud rate for the eUSCI which gets its clock from SMCLK
//	  UCA3BRW = 3; // 16 MHz / 3 = .1875 us per bit
	UCB1BRW = 3; // 12 MHz / 6 = .25 us per bit
	  // modulation is not used in SPI mode, so clear UCA3MCTLW
//	UCB1MCTLW = 0;
	P6SEL0 |= 0x38;
	P6SEL1 &= ~0x38;                      // configure P9.7, P9.5, and P9.4 as primary module function
	UCB1CTLW0 &= ~0x0001;                 // enable eUSCI module
	UCB1IE &= ~0x0003;                    // disable interrupts

//	spi_Open();


	clearStrip();					// clear the strip
}
//------------------------------------------------------------------------------
// Sets the color of a certain LED (0 indexed)
void setLEDColor(u_int p, u_char r, u_char g, u_char b, u_char w)
{
	leds[p].red = r;
	leds[p].green = g;
	leds[p].blue = b;
	leds[p].white = w;
}
//------------------------------------------------------------------------------
// Send colors to the strip and show them. Disables interrupts while processing.
void showStrip()
{
//	__bic_SR_register(GIE);       	// disable interrupts
	Interrupt_disableMaster();

	// send RGB color for every LED
	int i, j;
	for (i = 0; i < NUM_LEDS; i++){
		u_char rgb[4] = {leds[i].green, leds[i].red, leds[i].blue, leds[i].white};	// get RGB color for this LED

		// send green, then red, then blue, then white
		for (j = 0; j < 4; j++){
			u_char mask = 0x80;					// b1000000

			// check each of the 8 bits
			while(mask != 0){
				while ((UCB1IFG&0x0002)==0x0000);	// wait until empty to transmit

				if (rgb[j] & mask)
				{			// most significant bit first
//					SPI_transmitData(EUSCI_B1_MODULE, HIGH_CODE);
					UCB1TXBUF = HIGH_CODE;		// send 1
				}
				else
				{
				//	SPI_transmitData(EUSCI_B1_MODULE, LOW_CODE);
					UCB1TXBUF = LOW_CODE;		// send 0
				}

				mask >>= 1;						// check next bit
			}
		}
	}

	// send RES code for at least 50 us
	_delay_cycles(800);

//	__bis_SR_register(GIE);       	// enable interrupts
	Interrupt_enableMaster();
}
//------------------------------------------------------------------------------
// Clear the color of all LEDs (make them black/off)
void clearStrip(){
	fillStrip(0x00, 0x00, 0x00, 0x00);	// black
}
//------------------------------------------------------------------------------
// Fill the strip with a solid color. This will update the strip.
void fillStrip(u_char r, u_char g, u_char b, u_char w){
	int i;
	for (i = 0; i < NUM_LEDS; i++){
		setLEDColor(i, r, g, b, w);		// set all LEDs to specified color
	}
	showStrip();						// refresh strip
}
//------------------------------------------------------------------------------
void msDelay(uint32_t delay)
{
	uint32_t i;

	ms_timeout=0;
	while(!ms_timeout); // finish off last timeout period
	ms_timeout=0;
	for(i=0; i<delay; i++) {
		while(!ms_timeout);  // wait for ms timeout
		ms_timeout=0;
	}
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int InitLight(void)
{
	int err = 0;

	memset(leds, 0, sizeof(leds));

	// Set P1.0 to output direction to drive red LED
	GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN0);

	// Configure SysTick
	SysTick_enableModule();
	SysTick_setPeriod(16000);  // with a 68 MHz clock, this period is 1 ms
	SysTick_enableInterrupt();

	//    MAP_Interrupt_enableSleepOnIsrExit();
	MAP_Interrupt_enableMaster();

	initStrip();			// ***** HAVE YOU SET YOUR NUM_LEDS DEFINE IN WS2812.C? ******

	// set strip color red
	fillStrip(0xFF, 0x00, 0x00, 0x00);

	// show the strip
	showStrip();

	// gradually fill for ever and ever
	u_int numLEDs = 7;

	gradualFill(numLEDs, 0x0F, 0x00, 0x00, 0x00);		// red

	return err;
}

//int main(void)
//{
//	uint32_t MCLKfreq, SMCLKfreq;
//	uint16_t i;
//
//    WDT_A_holdTimer();
//
//    /* Configuring pins for peripheral/crystal usage */
//    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,
//            GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);
//
//    /* Setting the external clock frequency. This API is optional, but will
//     * come in handy if the user ever wants to use the getMCLK/getACLK/etc
//     * functions
//     */
//    CS_setExternalClockSourceFrequency(32000,48000000);
//
//    /* Starting HFXT in non-bypass mode without a timeout. Before we start
//     * we have to change VCORE to 1 to support the 48MHz frequency */
//    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
//    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
//    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
//    CS_startHFXT(false);
//
//    /* Initializing MCLK to HFXT (effectively 48MHz) */
//    MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
////    CS_setDCOFrequency (16000000); // set to 16 MHz (timing is set for this freq)
//
//    MCLKfreq=MAP_CS_getMCLK();  // get MCLK value
//
//    MAP_CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);  // set SMCLK to 12 MHz
//    SMCLKfreq=MAP_CS_getSMCLK();  // get SMCLK value to verify it was set correctly
//
//    // Set P1.0 to output direction to drive red LED
//    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN0);
//
//    // Configure SysTick
//
//    SysTick_enableModule();
//    SysTick_setPeriod(16000);  // with a 68 MHz clock, this period is 1 ms
//    SysTick_enableInterrupt();
//
//    //    MAP_Interrupt_enableSleepOnIsrExit();
//    MAP_Interrupt_enableMaster();
//
//    initStrip();			// ***** HAVE YOU SET YOUR NUM_LEDS DEFINE IN WS2812.C? ******
//
//	// set strip color red
//	fillStrip(0xFF, 0x00, 0x00, 0x00);
//
//	// show the strip
//	showStrip();
//
//	// gradually fill for ever and ever
//	u_int numLEDs = 7;
//	while(1){
//		gradualFill(numLEDs, 0x0F, 0x00, 0x00, 0x00);		// red
//		msDelay(3000);
//		gradualFill(numLEDs, 0x00, 0x0F, 0x00, 0x00);		// green
//		msDelay(3000);
//		gradualFill(numLEDs, 0x00, 0x00, 0x0F, 0x00);		// blue
//		msDelay(3000);
//		gradualFill(numLEDs, 0x0F, 0x00, 0x0F, 0x00);		// magenta
//		msDelay(3000);
//		gradualFill(numLEDs, 0x0F, 0x0F, 0x00, 0x00);		// yellow
//		msDelay(3000);
//		gradualFill(numLEDs, 0x00, 0x0F, 0x0F, 0x00);		// cyan
//		msDelay(3000);
//		gradualFill(numLEDs, 0x00, 0x00, 0x00, 0x0F);		// white
//		msDelay(5000);
//	}
//}
//------------------------------------------------------------------------------
void gradualFill(u_int n, u_char r, u_char g, u_char b, u_char w){
	int i;
	for (i = 0; i < n; i++){			// n is number of LEDs
		setLEDColor(i, r, g, b, w);
		showStrip();
//		_delay_cycles(50000);			// lazy delay
		msDelay(200);
	}
}
//------------------------------------------------------------------------------
void SpinningColor(u_char r, u_char g, u_char b, u_char w)
{
	static int a = 0, check = 0, counter = 0;
	int i = 0;

	for(i = 0; i < 7; i++)
	{
		if(BallBlink)
		{
			if(check)
				setLEDColor(i, r, g, b, w);
			else
				setLEDColor(i, 0x00, 0x00, 0x00, 0x4F);
		}
		else
		{
			setLEDColor(i, r, g, b, w);

			leds[a].red = 0x00;
			leds[a].green = 0x00;
			leds[a].blue = 0x00;
			leds[a].white = 0x4F;
		}


		showStrip();
	}


	if(BallBlink)
	{
		if(counter++ < 4)
			check = 0;
		else
			check = 1;

		if(counter >= 12)
			counter = 0;
	}
	else
	{
		if(a >= 6)
			a = 0;
		else
			a++;
	}
}
