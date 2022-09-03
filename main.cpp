/******************************************************************************


-------------------------------------------------------------------------------
        Project Name : AutoControlRaspberryfan                                 
        Author       : BDZNH                                                   
        Project URL  : https://github.com/BDZNH/AutoControlRaspberryFan        
        what is this : Auto control raspberry fan with 5V. Turn the fan        
                       when the temperaure is high than 45, turn off         
                       fan when the CPU temperature is lower than 39.        
-------------------------------------------------------------------------------



******************************************************************************/

/**
2020-06-21 修改，禁用日志输出， 减少对sd卡的访问
*/

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>	  //For getpid(),sleep()
#include <wiringPi.h> //wiringPiSetup(),
#include <softPwm.h>  //softPwmCreate(),softPwmWrite()
#include <string.h>
#include <ctime>

const unsigned int _FANPIN = 1; //使用gpio 1号脚，对应物理引角12号
const std::string TEMP_PATH = "/sys/class/thermal/thermal_zone0/temp";
//const std::string LOG_PATH = "/var/log/raspberrypiFanSpeed.log";
const std::string PID_PATH = "/var/run/autocontrolfan.pid";

//统一使用这个
const unsigned int MAX_TEMP = 52;
const unsigned int MIN_TEMP = 45;
const unsigned int CLOSE_TEMP = 41;

bool Fan_run_Flag = true;

inline int min(int x, int y)
{
	return x <= y ? x : y;
}

void GetCpuTempera(std::ifstream &fin, double &temp);
int initWiringPi();
void showInfo();
void SaveLog(double &temp, int FanSpeed, time_t &time_cur);
void Check_Flags(unsigned int &argc, char *argv[], bool &quiet);

void startOrStopFan(int Fan_Speed)
{
	if (Fan_Speed == 0 && Fan_run_Flag)
	{
		softPwmWrite(_FANPIN, 0);
		Fan_run_Flag = false;
	}
	else
	{
		softPwmWrite(_FANPIN, Fan_Speed);
		Fan_run_Flag = true;
	}
}

int main(int argc, char *argv[])
{

	bool quiet = false;

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "q") == 0)
			quiet = true;
	}

	if (!quiet)
		showInfo();
	// std::ofstream log(LOG_PATH.c_str());
	// if (!log.is_open())
	// {
	// 	std::cout << "Can't open file : " << LOG_PATH << std::endl;
	// }
	// log.close();
	std::ifstream fin(TEMP_PATH.c_str(), std::ios_base::in);
	if (!fin.is_open())
	{
		std::cout << "Can't open file : " << TEMP_PATH << std::endl;
		return -1;
	}
	std::ofstream pid(PID_PATH.c_str());
	if (!pid.is_open())
	{
		std::cout << "Can't open file : " << PID_PATH << std::endl;
	}
	pid << getpid() << std::endl;
	pid.close();
	pid.clear();
	time_t time_cur;
	double temp = 0;
	int Fan_Speed = 0;
	bool Forty_five_Flag = false;

	if (initWiringPi() < 0)
	{
		return -1;
	}

	while (true)
	{
		GetCpuTempera(fin, temp);
		// if (!quiet)
		// {
		// 	if (temp >= 42) //使用-q参数查看时，大于42度时用红色提示
		// 		std::cout << "Cpu temperature is : \033[0;31m" << temp << " `C \033[0m" << std::flush;
		// 	else
		// 		std::cout << "Cpu temperature is : \033[1;32m" << temp << " `C\033[0m" << std::flush;
		// }
		if (Forty_five_Flag)
		{
			if (temp < CLOSE_TEMP) //小于CLOSE_TEMP度时才关闭风扇
			{
				sleep(1); // 当温度从最大温度时降到关闭温度时10秒后再停止。
				Forty_five_Flag = false;
				Fan_Speed = 0;
				//softPwmWrite(_FANPIN, Fan_Speed);
				startOrStopFan(Fan_Speed);
				SaveLog(temp, Fan_Speed, time_cur);
			}
			else
			{
				SaveLog(temp, Fan_Speed, time_cur);
			}
			sleep(5);
		}
		else
		{
			if (temp < CLOSE_TEMP)
			{
				sleep(1);
				Fan_Speed = 0;
				//softPwmWrite(_FANPIN, Fan_Speed);
				startOrStopFan(Fan_Speed);
				SaveLog(temp, Fan_Speed, time_cur);
			}
			else if (temp >= MIN_TEMP && temp <= MAX_TEMP)
			{
				Fan_Speed = min(((((int)temp - 40) * 10) + 80), 100);
				// Fan_Speed = 80;
				//softPwmWrite(_FANPIN, Fan_Speed);
				startOrStopFan(Fan_Speed);
				SaveLog(temp, Fan_Speed, time_cur);
				sleep(5);
			}
			else if (temp > MAX_TEMP)
			{
				Fan_Speed = 100;
				//softPwmWrite(_FANPIN, Fan_Speed);
				startOrStopFan(Fan_Speed);
				Forty_five_Flag = true;
				SaveLog(temp, Fan_Speed, time_cur);
				sleep(5);
			}
		}
		sleep(1);
		std::cout << "\r";
	}
	return 0;
}

void GetCpuTempera(std::ifstream &fin, double &temp)
{
	fin >> temp;
	temp = temp / 1000.0;
	fin.clear();
	fin.seekg(0, std::ios::beg);
}

int initWiringPi()
{
	if (wiringPiSetup() != 0)
	{
		std::cout << "WiringPi setup failed" << std::flush << " \r";
		return -1;
	}
	if (softPwmCreate(_FANPIN, 0, 100) != 0)
	{
		std::cout << "softPwmcreat setup failed" << std::flush << " \r";
		return -2;
	}
	else
	{
		std::cout << "set gpio " << _FANPIN << " range 0 to 100" << std::endl;
		softPwmWrite(_FANPIN, 100); // 初始化时启动风扇运行.
	}
	return 0;
}

void Check_Flags(unsigned int &argc, char *argv[], bool &quiet)
{
}

void showInfo()
{
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	std::cout << "        Project Name : AutoControlRaspberryfan                                 " << std::endl;
	//std::cout << "        Author       : BDZNH                                                   " << std::endl;
	//std::cout << "        Project URL  : https://github.com/BDZNH/AutoControlRaspberryFan        " << std::endl;
	std::cout << "        what is this : Auto control raspberry fan with 5V. Turn the fan        " << std::endl;
	std::cout << "                       when the temperaure is high than" << MIN_TEMP << ", turn off         " << std::endl;
	std::cout << "                       MAX: " << MAX_TEMP << "| MIN: " << MIN_TEMP << " | CLOSE: " << CLOSE_TEMP << std::endl;
	std::cout << "                       fan when the CPU temperature is lower than." << CLOSE_TEMP << std::endl;
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	std::cout << "\n\n\n"
			  << std::endl;
}

void SaveLog(double &temp, int Fan_Speed, time_t &time_cur)
{
	// log.open(LOG_PATH.c_str(), std::ios_base::out);
	// if (!log.is_open())
	// {
	// 	std::cout << "Can't open file : " << LOG_PATH << std::endl;
	// }
	// time(&time_cur);
	// log << ctime(&time_cur) << "CPU temperature is : " << temp << " `C\nSet fan speed to " << Fan_Speed << std::endl;
	// log.close();
	// log.clear();

	time(&time_cur);
	// std::cout << ctime(&time_cur) << "CPU temperature is : " << temp << " `C. Set fan speed to " << Fan_Speed << std::endl;

	if (temp >= 42)
	{
		std::cout << ctime(&time_cur) << "Cpu temperature is : \033[0;31m" << temp << " `C \033[0m. Set fan speed to " << Fan_Speed << std::flush;
	} 

	else
	{
		std::cout << ctime(&time_cur) << "Cpu temperature is : \033[1;32m" << temp << " `C\033[0m. Set fan speed to " << Fan_Speed << std::flush;
	}
}