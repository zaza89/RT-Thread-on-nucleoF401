
/*
 *	Author:		GalaIO
 *	Date:			2015-7-26 10:12 AM
 *	Description:
 *			Re package RTC configuration.
 *			-Define the week description macro.
 *					MONDAY		0
 *						  ....
 *					SUNDAY		6
 *			-init the RTC with Bak_Date_Time[] = {2015,7,23,1,55,00}.
 *				Init_RTC(void)
 *			-update RTC as the specific year month day hour minute second.
 *				Time_Update(uint16_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t min,uint8_t sec)
 *			-refresh the time for rtc_timer in sram.
 *				Time_Get(void)
 *			-handler timer and date tools.
 *				-check date param, correct return 0, erro return 1.
 *					Check_Date(uint16_t year,uint8_t month,uint8_t day)
 *				-check time param, correct return 0, erro return 1.
 *					Check_Time(uint8_t hour,uint8_t min,uint8_t sec)
 *				-get week info.
 *					RTC_Get_Week(uint16_t year,uint16_t month,uint16_t day)
 *				-judge a leap year.
 *					Is_Leap_Year(u16 year)
 *		
**/
#include "bsp_rtc.h"

/*
 *  define and declare.
**/


//得到Date_t的两个父类，一个叫RTC_TimeTypeDef；另一个是RTC_DateTypeDef。
#define RDT_GET_DATETYPE(pRDT)	((RTC_DateTypeDef *)&(pRDT->week))
#define RDT_GET_TIMETYPE(pRDT)	((RTC_TimeTypeDef *)&(pRDT->hour))


//检查输入时间和日期的参数，分开，注意我们默认是24小时制哦
//输出0，正确；输出1，错误。
uint8_t Check_Date(uint16_t year,uint8_t month,uint8_t day);

uint8_t Check_Time(uint8_t hour,uint8_t min,uint8_t sec);

//获得现在是星期几
//功能描述:输入公历日期得到星期(只允许2000-2099年)
//输入参数：公历年月日 
//返回值：星期号																						 
uint8_t RTC_Get_Week(uint16_t year,uint16_t month,uint16_t day);
//判断是否是闰年函数
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
u8 Is_Leap_Year(u16 year);


//月份   1  2  3  4  5  6  7  8  9  10 11 12  total
//闰年   31 29 31 30 31 30 31 31 30 31 30 31	 366
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31   365
//设月30日，那每个月的偏差时多少。
//闰年   +1 -1 +1  0 +1  0 +1 +1  0 +1  0 +1	 366
//非闰年 +1 -2 +1  0 +1  0 +1 +1  0 +1  0 +1   365
//月份数据表											 
const int8_t	Day_INC_bMon[] = 			{0,1,-1,0,0,1,1,2,3,3,4,4,5};	
const int8_t	Day_INC_bMon_Leap[] = {0,1, 0,1,1,2,2,3,4,4,5,5,6};	  

const uint8_t Month_Table[] = 			{0,31,28,31,30,31,30,31,31,30,31,30,31};	//平年的月份日期表
const uint8_t Month_Table_Leap[] =  {0,31,29,31,30,31,30,31,31,30,31,30,31};		//闰年的月份日期表

//以2000年1月1日星期6为基准来识别星期。同时只支持从2000-2099年的日期，
//修改基准日期能改变日期，但只能数100年（RTC限制的）。
const uint16_t Date_Benchmark[] = {2000,1,1,5};

//back Time and date.
const uint16_t Bak_Date_Time[] = {2015,7,23,1,55,00};

const char DATE_WEEK_STR[][10] = {"1221","MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY","SUNDAY"};


void setDefaultRTC(){
		RDT_t rdt;
		rdt.year = Bak_Date_Time[0];
		rdt.month = Bak_Date_Time[1];
		rdt.day = Bak_Date_Time[2];
		rdt.hour = Bak_Date_Time[3];
		rdt.min = Bak_Date_Time[4];
		rdt.sec = Bak_Date_Time[5];
		RDT_sync(&rdt);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
** 函数名称: Init_RTC
** 功能描述: RTC初始化
** 参数描述：无
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
u8 Init_RTC(void)
{
	RTC_InitTypeDef RTC_InitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); //使能PER时钟
																	//使能PWR和BKP外设时钟   		
	PWR_BackupAccessCmd(ENABLE);									//使能RTC和后备寄存器访问 
	
	if(RTC_ReadBackupRegister(RTC_BKP_DR0)!=0x5555)						//从指定的后备寄存器中读出数据，判断是否为第一次配置
	{																
		RCC_LSEConfig(RCC_LSE_ON);									//使能外部低速时钟 32.768KHz
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)			//检查指定的RCC标志位设置与否,等待低速晶振就绪
  		{}
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);						//设置RTC时钟(RTCCLK),选择LSE作为RTC时钟    
		RCC_RTCCLKCmd(ENABLE);										//使能RTC时钟  
		
				
    RTC_InitStructure.RTC_AsynchPrediv = 0x7F;//RTC异步分频系数(1~0X7F) 128-1
    RTC_InitStructure.RTC_SynchPrediv  = 0xFF;//RTC同步分频系数(0~7FFF) 256-1
    RTC_InitStructure.RTC_HourFormat   = RTC_HourFormat_24;//RTC设置为24小时制
    RTC_Init(&RTC_InitStructure);
		
		//设置最原始时间，就像手机没有电池后，时间会变成厂家预设的
		setDefaultRTC();
				
		RTC_WriteBackupRegister(RTC_BKP_DR0, 0x5555);					//向指定的后备寄存器中写入用户程序数据0X5555做判断标志										
	}																 	
	//不是第一次配置 继续计时
	
	
	return 0; //ok		
}


//检查输入时间和日期的参数，分开，注意我们默认是24小时制哦
//输出0，正确；输出1，错误。
uint8_t Check_Date(uint16_t year,uint8_t month,uint8_t day){
	//assert the year .
	if(year>Date_Benchmark[0]+99 || year<Date_Benchmark[0]){
		return 1;
	}
	//assert the month .
	if(month>12 || month<1){
		return 1;
	}
	//assert the day .
	if(Is_Leap_Year(year)){
		if(day<1 || day>Month_Table[month]){
			return 1;
		}
	}else{
		if(day<1 || day>Month_Table_Leap[month]){
			return 1;
		}
	}
	return 0;
}

uint8_t Check_Time(uint8_t hour,uint8_t min,uint8_t sec){
	if(hour>23){
		return 1;
	}
	if(min>59){
		return 1;
	}
	if(sec>59){
		return 1;
	}
	return 0;
}


//sync the date and time to RTC.
//@param pRDT		a point to a RTC struct.
//return 0:success; other:faild.
//notes: the pRDT->year must whtin Date_Benchmark[0] - Date_Benchmark[0]+99, in 100 range.
uint8_t RDT_sync(RDT_t *pRDT)
{ 
	uint8_t week;
	if(Check_Date(pRDT->year,pRDT->month,pRDT->day) || Check_Time(pRDT->hour,pRDT->min,pRDT->sec)){
		return 1;
	}
	week = RTC_Get_Week(pRDT->year,pRDT->month,pRDT->day);
	if(week>SUNDAY) return 1;
	//amend the year param.
	pRDT->year = pRDT->year - Date_Benchmark[0];
	//add week .
	pRDT->week = week+1;
	//add am or pm.
	pRDT->AMoPM = pRDT->hour<12?RTC_H12_AM:RTC_H12_PM;
	
	//translate RDT_t to (RTC_DateTypeDef *) and (RTC_TimeTypeDef *)
	do{
		RTC_DateTypeDef *dateType = RDT_GET_DATETYPE(pRDT);
		RTC_TimeTypeDef *timeType = RDT_GET_TIMETYPE(pRDT);
		dateType->RTC_Year = pRDT->year;
		//向RTC设置时间和日期
		RTC_SetTime(RTC_Format_BIN,timeType);
		RTC_SetDate(RTC_Format_BIN,dateType);
	}while(0);
	
	return 0;
}

//gain the newest date and time from RTC.
//@param pRDT	 the RTC struct can contain the whole info.
//return void.
void RDT_gain(RDT_t *pRDT){
	//向RTC获取时间和日期
	RTC_DateTypeDef *dateType = RDT_GET_DATETYPE(pRDT);
	RTC_TimeTypeDef *timeType = RDT_GET_TIMETYPE(pRDT);
	RTC_GetTime(RTC_Format_BIN,timeType);
	RTC_GetDate(RTC_Format_BIN,dateType);
	pRDT->year = dateType->RTC_Year;
	pRDT->year += Date_Benchmark[0];

	//获取 RTC的亚秒, 注意亚秒是从高到低的，从1000 -- 0
	pRDT->sub_sec = 1.0*RTC_GetSubSecond()*3.9;
	
	return ;
}


//获得现在是星期几
//功能描述:输入公历日期得到星期(只允许2000-2099年)
//输入参数：公历年月日 
//返回值：星期号 0-6/mon-sun if you want 1-7/mon-sun, you must plus 1;																						 
uint8_t RTC_Get_Week(uint16_t year,uint16_t month,uint16_t day)
{	
	uint32_t	temp,days;
	//assert the year month day.
	if(Check_Date(year,month,day)){
		return SUNDAY+1;
	}
	//count the number of leap in the range.
	temp=year/4-year/100+year/400-(Date_Benchmark[0]/4-Date_Benchmark[0]/100+Date_Benchmark[0]/400);
	if(Is_Leap_Year(year)){
		temp--;
	}
	days=temp*366+(year-Date_Benchmark[0]-temp-1)*365;		//得到除边界外 年份的天数和
	//计算边界 天数.
	//右边界
	if(Is_Leap_Year(year)){
		days+=(month-1)*30+Day_INC_bMon_Leap[month-1]+day;
	}else{
		days+=(month-1)*30+Day_INC_bMon[month-1]+day;
	}
	//左边界
	if(Is_Leap_Year(Date_Benchmark[0])){
		days+=(12-Date_Benchmark[1])*30+Day_INC_bMon_Leap[12]-Day_INC_bMon_Leap[Date_Benchmark[1]]+Month_Table_Leap[Date_Benchmark[1]]-Date_Benchmark[2];
	}else{
		days+=(12-Date_Benchmark[1])*30+Day_INC_bMon[12]-Day_INC_bMon[Date_Benchmark[1]]+Month_Table[Date_Benchmark[1]]-Date_Benchmark[2];
	}
	days+=Date_Benchmark[3];
	return days%7;
}
//判断是否是闰年函数
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
u8 Is_Leap_Year(u16 year)
{			  
	if(year%4==0) //必须能被4整除
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1;//如果以00结尾,还要能被400整除 	   
			else return 0;   
		}else return 1;   
	}else return 0;	
}


