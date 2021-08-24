#include <iostream>

#include "CSerialPort/SerialPort.h"
#include "CSerialPort/SerialPortInfo.h"

#include <time.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#define imsleep(microsecond) Sleep(microsecond) // ms
#else
#include <unistd.h>
#define imsleep(microsecond) usleep(1000 * microsecond) // ms
#endif

#include <vector>
using namespace itas109;
using namespace std;

int countRead = 0;
//指令
const char cmd_ic[] = "ARE_YOU_ICOPY?\r\n";


class slotpath : public has_slots<>
{
public:
	slotpath(CSerialPort* sp)
	{
		p_sp = sp;
	};

	int readline(char* str_no_line_end, int str_size, int outtime) {
		memset(str_no_line_end, 0x00, str_size);
		int recLen = 0;
		int index = 0;
		bool getr = false;
		clock_t start, finish;
		start = clock();

		for (;;)
		{
			char inbyte[1] = { 0 };
			recLen = p_sp->readData(inbyte, 1);
			if (recLen == 1) {
				//获得了一个byte
				if (inbyte[0] == '\r' && getr == false) {//在未获得r时获得了r，则标记
					//std::cout << "获得r" << std::endl;
					getr = true;
				}
				else if (getr) {//获得r的时候

					if (inbyte[0] == '\n') {
						//如果获得了n则意味着结束了
						//std::cout << "rn都有，返回" << std::endl;
						return (index);
					}
					else {
						//如果没有获得n则这个包出问题了，抛弃
						//std::cout << "只有r，下一个字符不是n，抛弃包" << std::endl;
						memset(str_no_line_end, 0x00, str_size);
						int index = 0;
					}
				}
				else {//rn都没有的时候
					str_no_line_end[index] = inbyte[0];
					index++;
				}
			}
			else if (recLen != 0) {
				std::cout << "!!!port err!!!" << std::endl;
				return -1;
			}
			finish = clock();
			double dTime = (double)(finish - start) / CLOCKS_PER_SEC;
			if (outtime < dTime * 1000) {
				return -2;
			}
		}
	}

private:
	slotpath() {};

private:
	CSerialPort* p_sp;
};




int main()
{
	bool updatetimeflag = false;
	int index = -1;
	std::string portName;
	vector<SerialPortInfo> m_availablePortsList;

	CSerialPort sp;

	slotpath receive(&sp);

	m_availablePortsList = CSerialPortInfo::availablePortInfos();

	std::cout << "ports list: " << std::endl;

	for (int i = 0; i < m_availablePortsList.size(); i++)
	{
		std::cout << i 
			<< " - " << m_availablePortsList[i].portName 
			<< " " << m_availablePortsList[i].description
			//<< " || id:" << m_availablePortsList[i].hardwareId
			//<< " || mf:" << m_availablePortsList[i].manufactor
			<< std::endl;
	}

	if (m_availablePortsList.size() == 0)
	{
		std::cout << "No valid port，check icopy connction" << std::endl;
	}
	else
	{
		std::cout << std::endl;
		//从0到m_availablePortsList.size() - 1
		//开始寻找正确的设备
		std::cout << "find icopy...";
		std::cout.unsetf(ios::left); //取消对齐方式，用缺省right方式 
		std::cout.fill('.'); //设置填充方式 
		std::cout.width(50-13); //设置宽度，只对下条输出有用 
		for (index = 0;index < m_availablePortsList.size(); index++) {
			//string::size_type m = m_availablePortsList[index].hardwareId.find("EVSERIAL", 0);
			string::size_type m = m_availablePortsList[index].hardwareId.find("USB\\VID_0525&PID_A4A7", 0);
			if ((int)m > -1) {
				std::cout << "found" <<std::endl;
				break;
			}
		}
		if (index == m_availablePortsList.size()) {
			std::cout << "not found, please check your connection"<< std::endl;
			std::cout << std::endl << std::endl;
			std::cout << "END OF PROGRAM,PRESS ANY KEY TO EXIT!";
			getchar();
			return 0;
		}
		portName = m_availablePortsList[index].portName;
		std::cout << "opening port: " << portName << "...";
		std::cout.unsetf(ios::left); //取消对齐方式，用缺省right方式 
		std::cout.fill('.'); //设置填充方式 
		std::cout.width(50-17-portName.length()); //设置宽度，只对下条输出有用 

		sp.init(portName);//windows:COM1 Linux:/dev/ttyS0

		sp.open();

		if (sp.isOpened())
		{
			std::cout <<"success" << std::endl;
		}
		else
		{
			std::cout <<" failed" << std::endl;
		}

		std::cout << "Identify the target...";
		std::cout.unsetf(ios::left); //取消对齐方式，用缺省right方式 
		std::cout.fill('.'); //设置填充方式 
		std::cout.width(50 - 22); //设置宽度，只对下条输出有用 
		//发送问询指令

		sp.writeData(cmd_ic, sizeof(cmd_ic));
		char instr[1024];
		int times = 0;
		int result = receive.readline(instr, sizeof(instr), 2000);
		for (;result == -2 && times < 3;times++) {
			std::cout << "error" << std::endl;
			std::cout << "Identify the target...";
			std::cout.unsetf(ios::left); //取消对齐方式，用缺省right方式 
			std::cout.fill('.'); //设置填充方式 
			std::cout.width(50 - 22); //设置宽度，只对下条输出有用 
			result = receive.readline(instr, sizeof(instr), 2000);
		}
		if (times == 3) {
			std::cout << "error" << std::endl;
		}
		else {
			//判断回应是否正常
			string::size_type m = string(instr).find("YES!ICOPY!", 0);
			if ((int)m > -1) {
				std::cout << "ok" << std::endl;
				updatetimeflag = true;
			}
			else {
				std::cout << "error" << std::endl;
			}	
		}
		imsleep(1000);
		while (updatetimeflag) {
			char char_time[50] = { 0 };
			SYSTEMTIME sysTime;
			ZeroMemory(&sysTime, sizeof(sysTime));
			GetLocalTime(&sysTime);
			sprintf_s(char_time, "%04d-%02d-%02d-%02d-%02d-%02d\0", sysTime.wYear, sysTime.wMonth,
				sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
			std::cout << "send time...";
			std::cout << char_time;
			sprintf_s(char_time, "time=%04d-%02d-%02d-%02d-%02d-%02d\r\n\0", sysTime.wYear, sysTime.wMonth,
			sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
			sp.writeData(char_time, strlen(char_time));
			std::cout.unsetf(ios::left); //取消对齐方式，用缺省right方式 
			std::cout.fill('.'); //设置填充方式 
			std::cout.width(50 - 12 - 19); //设置宽度，只对下条输出有用 
			//开始判断返回值
			result = receive.readline(instr, sizeof(instr), 2000);
			for (times = 0;result == -2 && times < 3;times++) {
				result = receive.readline(instr, sizeof(instr), 2000);
			}
			if (times == 3) {
				std::cout << "error_nc" << std::endl;
			}
			else {
				//判断回应是否正常
				string::size_type m = string(instr).find("SET!TIME!OK!", 0);
				if ((int)m > -1) {
					std::cout << "ok" << std::endl;
					break;
				}
				else {
					std::cout << "error_" << m << std::endl;
				}
			}
			imsleep(2000);
		}
		sp.close();
	}
	std::cout << std::endl << std::endl;
	std::cout << "END OF PROGRAM,PRESS ENTER TO EXIT!";
	getchar();
	return 0;
}