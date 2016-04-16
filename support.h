#ifndef SUPPORT_H_
#define SUPPORT_H_

#define	YES				1
#define NO				0

#define RED				1
#define BLUE			2
#define GREEN			3

#define	IDLE			0
#define	GET_IPD			1
#define	LAUNCH_WEBSITE	2
#define	GET_DATA		3

#define BUFFER_SIZE		10000

#define OUTPUT_PIN	(0x40)			// Set to whatever UCB3SIMO is on your processor (Px.6 here)
#define NUM_LEDS	(7)			// NUMBER OF LEDS IN YOUR STRIP

// Transmit codes
#define HIGH_CODE	(0xF0)			// b11110000
#define LOW_CODE	(0xC0)			// b11000000

// Useful typedefs
typedef unsigned char u_char;		// 8 bit
typedef unsigned int u_int;			// 16 bit



#ifdef MAIN_PROGRAM
#define XTRN
#else
#define XTRN extern
#endif

int InitFunction(void);
int PutInClientMode(void);
void my_puts(char *_ptr);
char DesiredString[100];
int CheckForString(char *letter);
void ESP8266_Send(char *_sptr);
int InitFunction_Client(void);
int WaitForResponse_String(char *strToFind, int okToErase);
int ParseNIST_Data(int beginningByte, int endingByte, char *buf);
int ParseWUNDER_Data(int beginningByte, int endingByte, char *buf, int index);
int GetandSetTime(void);
int GetWUNDERdata(int index);
int getDataBetweenQuotationMarks(char *ptr, char dataReturned[80]);
int DisplayInit(void);
void Delay1ms(uint32_t n);
void DelayWait10ms(uint32_t n);
int spi_Open(void);
int BallColorDecision(void);
int LaunchWebsite(int channel);
void SpinningColor(u_char r, u_char g, u_char b, u_char w);

//Initialize the lights. Highlevel
int InitLight(void);

// Configure processor to output to data strip
void initStrip(void);

// Send colors to the strip and show them. Disables interrupts while processing.
void showStrip(void);

// Set the color of a certain LED
void setLEDColor(u_int p, u_char r, u_char g, u_char b, u_char w);

// Clear the color of all LEDs (make them black/off)
void clearStrip(void);

// Fill the strip with a solid color. This will update the strip.
void fillStrip(u_char r, u_char g, u_char b, u_char w);

void gradualFill(u_int n, u_char r, u_char g, u_char b, u_char w);

void msDelay(uint32_t delay);

XTRN char ESPbuffer[BUFFER_SIZE], DesiredStrings[2][100], DataOUT[2000];
XTRN uint16_t U0RXData, U2RXData;
XTRN uint32_t SMfreq, MCLKfreq, ACLKfreq;
XTRN int bufIDx, RXFlag, CurrentState, TimeOutTripped, NewData, Refresh, HTML_Update, HTML_LightUpdate;
XTRN int RTC_tick, LightUpdate, BallColor, BallBlink, IPDChannel;
XTRN RTC_C_Calendar newTime;
XTRN char strToFind[10][20];
XTRN int spaces[10], One_Day, Five_Day, Two_Day, ScrollingTick, LightTimer, WeatherUpdate;
XTRN int BallColorMASTER, BallBlinkMASTER;

typedef struct{
	unsigned int fahrenheitHIGH;
	unsigned int fahrenheitLOW;
	unsigned int  avehumidity;
	char conditions[30];
	unsigned int avewind;
	char windDirection[5];
}T_WeatherData;

typedef struct{
	int hour;
	int minute;
	int sec;
	int day;
	char dayString[4];
	int month;
	int year;
}T_TimeData;

typedef struct {
	u_char red;
	u_char green;
	u_char blue;
	u_char white;
} LED;

XTRN uint8_t ms_timeout;
XTRN uint32_t ms10Timer;
XTRN LED leds[NUM_LEDS];
XTRN T_WeatherData Weather[4];
XTRN T_TimeData TimeDate;

#endif /* SUPPORT_H_ */
