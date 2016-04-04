#ifndef SUPPORT_H_
#define SUPPORT_H_

#define	IDLE			0
#define	GET_IPD			1
#define	LAUNCH_WEBSITE	2
#define	GET_DATA		3

#define BUFFER_SIZE		1750

#ifdef MAIN_PROGRAM
#define XTRN
#else
#define XTRN extern
#endif

int InitFunction(void);
void PutInClientMode(void);
void my_puts(char *_ptr);
char DesiredString[100];
int CheckForString(char *letter);
void ESP8266_Send(char *_sptr);
int InitFunction_Client(void);
int WaitForResponse_String(char *strToFind, char *returnPtr);
int ParseData(int beginningByte, int endingByte, char *buf);
int GetandSetTime(void);


XTRN char ESPbuffer[2048], tempBuf[2048], DesiredStrings[2][100], DataOUT[2038];
XTRN uint16_t U0RXData, U2RXData;
XTRN uint32_t SMfreq, MCLKfreq, ACLKfreq;
XTRN int bufIDx, RXFlag, CurrentState, TimeOutTripped, NewData, Refresh;
XTRN int RTC_tick;
XTRN RTC_C_Calendar newTime;

typedef struct{
	double temp;
	double  rh;
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

XTRN T_WeatherData Weather;
XTRN T_TimeData TimeDate;



#endif /* SUPPORT_H_ */
