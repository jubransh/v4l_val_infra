#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <fstream>


#define LOG_INFO 1
#define LOG_DEBUG 2
#define LOG_WARNING 3
#define LOG_ERROR 4
#define DEFUALT_SEVERITY LOG_INFO

#define LOG_INFO_STR    "[INFO]   "
#define LOG_DEBUG_STR   "[DEBUG]  "
#define LOG_WARNING_STR "[WARNING]"
#define LOG_ERROR_STR   "[ERROR]  "

class Logger {
    string filePath;
    int loggerSeverity = LOG_INFO;
	ofstream logFile;
	std::mutex loggingMutex;
	bool isFileOpend;
	bool enableConsoleLog;
    
    Logger() {
		isFileOpend = false;
		enableConsoleLog = true;
    }
	~Logger() = default;
	Logger(const Logger&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(Logger&&) = delete;
	Logger& operator=(const Logger&) = delete;

	void enableConsoleLogs(bool consoleLogsEnabled) {
		enableConsoleLog = consoleLogsEnabled;
	}

	void setLoggerPath(string fp) {
		const std::lock_guard<std::mutex> lock(loggingMutex);
		if (isFileOpend) {
			logFile.close();
			filePath = fp;
			logFile.open(filePath, std::ios_base::app);
			if (logFile.fail())
			{
				cout << "Cannot open file: " << filePath << endl;
			}
		}
		else {
			filePath = fp;
			logFile.open(filePath, std::ios_base::app);
			if (logFile.fail())
			{
				cout << "Cannot open file: " << filePath << endl;
				throw std::runtime_error("Cannot open file: " + filePath);
			}
			isFileOpend = true;
		}
	}

	void setLoggerSeverity(int severity) {
		loggerSeverity = severity;
	}

	string intToStr(int i) {
		return std::to_string(i);
	}

	string getCurrentTime()
	{
		using namespace std::chrono;

		// get current time
		auto now = system_clock::now();

		// get number of milliseconds for the current second
		// (remainder after division into seconds)
		auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

		// convert to std::time_t in order to convert to std::tm (broken time)
		auto timer = system_clock::to_time_t(now);

		// convert to broken time
		std::tm bt = *std::localtime(&timer);

		std::ostringstream oss;

		oss << std::put_time(&bt, "%H:%M:%S"); // HH:MM:SS
		oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
		string day = intToStr(bt.tm_mday);
		string month = intToStr(bt.tm_mon + 1);
		string year = intToStr(bt.tm_year + 1900);
		string currentDate = day + "/" + month + "/" + year;

		return currentDate + " " + oss.str();
	}

public:

	/// <summary> To get a sattic instance of the "Logger" calss </summary>
	/// <returns> instance of the "Logger" calss </returns> 
	static Logger& getLogger() {
		static Logger instance;
		return instance;
	}

	/// <summary> Configure the logger output file path, the logger severity level and the console logging </summary>
	/// <param name="output_path">To set the logger output file path</param>
	/// <param name="severityLevel"> To set the logger severity level </param>
	/// <param name="consoleLogsEnabled"> To enable/ disable logging to the console </param>
	void configure(string output_path, int severityLevel, bool consoleLogsEnabled) {
		setLoggerPath(output_path);
		setLoggerSeverity(severityLevel);
		enableConsoleLogs(consoleLogsEnabled);
	}

	/// <summary> Configure the logger output file path and the logger severity level, console logging will be disabled</summary>
	/// <param name="output_path">To set the logger output file path</param>
	/// <param name="severityLevel"> To set the logger severity level </param>
	void configure(string output_path, int severityLevel) {
		configure(output_path, severityLevel, false);
	}

	/// <summary> Configure the logger output file path, the console logging will be disabled, and the logger severity level will be "LOG_INFO" as a default</summary>
	/// <param name="output_path">To set the logger output file path</param>
	void configure(string output_path) {
		configure(output_path, LOG_INFO);
	}

	/// <summary> To log a message with a certain level of severity </summary>
	/// <param name="message"> The log message </param>
	/// <param name="severity"> The log severity </param>
	void log(string message, int severity) {
		string currentTime = getCurrentTime();
		const std::lock_guard<std::mutex> lock(loggingMutex);
		if (severity >= loggerSeverity) {
			string severity_str;

			switch (severity)
			{
			case 1:
				severity_str = LOG_INFO_STR;
				break;
			case 2:
				severity_str = LOG_DEBUG_STR;
				break;
			case 3:
				severity_str = LOG_WARNING_STR;
				break;
			case 4:
				severity_str = LOG_ERROR_STR;
				break;
			default:
				severity_str = "";
			}
			if (enableConsoleLog) {
				cout << currentTime << "   " << severity_str << "   " << message << endl;
			}
			if (isFileOpend) {
				logFile << currentTime << "   " << severity_str << "   " << message << endl;
			}
		}
	}

	/// <summary> To log a message with a defualt level of severity </summary>
	/// <param name="message"> The log message </param>
	/// <param name="severity"> The log severity </param>
	void log(string message) {
		log(message, DEFUALT_SEVERITY);
	}

	/// <summary> To log a message with a certain level of severity </summary>
	/// <param name="message"> The log message </param>
	/// <param name="severity"> The log severity </param>
	void log(string message, string tag, int severity) {
		string currentTime = getCurrentTime();
		const std::lock_guard<std::mutex> lock(loggingMutex);
		if (severity >= loggerSeverity) {
			string severity_str;

			switch (severity)
			{
			case 1:
				severity_str = LOG_INFO_STR;
				break;
			case 2:
				severity_str = LOG_DEBUG_STR;
				break;
			case 3:
				severity_str = LOG_WARNING_STR;
				break;
			case 4:
				severity_str = LOG_ERROR_STR;
				break;
			default:
				severity_str = "";
			}
			if (enableConsoleLog) {
				cout << currentTime << "   " << severity_str << "   " << "[" << tag << "]" << "   " << message << endl;
			}
			if (isFileOpend) {
				logFile << currentTime << "   " << severity_str << "   " << "[" << tag << "]" << "   " << message << endl;
			}
		}
	}
	void log(string message, string tag) 
	{
		log(message, tag,LOG_INFO);
	}

	/// <summary> To close the output file </summary>
	void close() {
		const std::lock_guard<std::mutex> lock(loggingMutex);
		if (isFileOpend) {
			logFile.close();
			filePath = "";
			isFileOpend = false;
		}
	}
};
