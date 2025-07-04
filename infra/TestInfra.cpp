using namespace std;
#include <math.h> /* round, floor, ceil, trunc */
#include <string>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <chrono>
#include "CameraInfra.cpp"
#include "monitor.cpp"
#include <sstream>
#include <fstream>
#include <thread>
#include "pg.cpp"
#include "cg.cpp"
#include "FrameAnalyzer.cpp"

#include <unistd.h>
#include <limits.h>

#include <iostream>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <sys/stat.h> // stat
#include <errno.h>	  // errno, ENOENT, EEXIST
#if defined(_WIN32)
#include <direct.h> // _mkdir
#endif

Profile running_profile;

int RawDataMAxSize = 10000000;
bool calcSystemTSMetrics = false;

string sid = TimeUtils::getDateandTime();
bool is_tests_res_created = false;
bool collectFrames = false;
bool depth_collectFrames = false;
bool ir_collectFrames = false;
bool color_collectFrames = false;
bool imu_collectFrames = false;

vector<Frame> depthFramesList, irFramesList, colorFramesList, accelFrameList, gyroFrameList;
vector<float> cpuSamples;
vector<float> memSamples;
double memoryBaseLine = 0;
vector<float> asicSamples;
vector<float> projectorSamples;
FrameAnalyzer fa;
bool isContentTest = false;

string exec(const char *cmd)
{
	array<char, 128> buffer;
	string result;
	string stderrcmd=string(cmd) + " 2>&1";
	unique_ptr<FILE, decltype(&pclose)> pipe(popen( stderrcmd.c_str(), "r"), pclose);
	if (!pipe)
	{
		throw runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}
	return result;
}

string getDriverVersion()
{
	string command = "modinfo d4xx";
	string str = exec(command.c_str());
	std::string delimiter = "\n";
	//Skip the first line
	str = str.substr(str.find(delimiter) + 1, str.length());

	//keep only the first line
	str = str.substr(0, str.find(delimiter));
	//Skip the "version:"
	auto nextIndex = str.find(" ");
	str = str.substr(nextIndex, str.length());

	//navigate to the total (nummeric value) and skip the spaces
	while (str.find(" ") == 0)
	{
		str = str.substr(1, str.length());
	}
	return str;
}

string gethostIP()
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}
bool stringIsInVector(string str, vector<string> vect)
{
	for (int i = 0; i < vect.size(); i++)
	{
		if (str == vect[i])
			return true;
	}
	return false;
}

void initFrameLists()
{
	depthFramesList.clear();
	irFramesList.clear();
	colorFramesList.clear();
	accelFrameList.clear();
	gyroFrameList.clear();
}

void initSamplesList()
{
	cpuSamples.clear();
	memSamples.clear();
	asicSamples.clear();
	projectorSamples.clear();
}

Profile currDepthProfile, currIRProfile, currColorProfile, currIMUProfile;
string streamComb;
void setCurrentProfiles(vector<Profile> ps)
{
	currDepthProfile = Profile();
	currDepthProfile.fps = 0;
	currIRProfile = Profile();
	currIRProfile.fps = 0;
	currColorProfile = Profile();
	currColorProfile.fps = 0;
	currIMUProfile = Profile();
	currIMUProfile.fps = 0;
	streamComb = "";
	string depthStr = "";
	string irStr = "";
	string colorStr = "";
	string imuStr = "";
	for (int i = 0; i < ps.size(); i++)
	{
		Profile p = ps[i];
		switch (p.streamType)
		{
		case StreamType::Depth_Stream:
			currDepthProfile = p;
			depthStr = p.GetText();
			break;
		case StreamType::IR_Stream:
			currIRProfile = p;
			irStr = p.GetText();
			break;
		case StreamType::Color_Stream:
			currColorProfile = p;
			colorStr = p.GetText();
			break;
		case StreamType::Imu_Stream:
			currIMUProfile = p;
			imuStr = p.GetText();
			break;
		default:
			break;
		}
	}
	if (depthStr != "")
		streamComb += depthStr + "\n";
	if (irStr != "")
		streamComb += irStr + "\n";
	if (colorStr != "")
		streamComb += colorStr + "\n";
	if (imuStr != "")
		streamComb += imuStr + "\n";
}

void addToPnpList(Sample s)
{
	cpuSamples.push_back(s.Cpu_per);
	memSamples.push_back(s.Mem_MB - memoryBaseLine);
}
void write_to_bin_file(string filePath, uint8_t *ptr, size_t len)
{
    try
    {
        ofstream fp;
        fp.open(filePath.c_str(), ios ::binary);
        fp.write((char *)ptr, len);
        fp.close();
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << endl;
    }
}

class FileUtils
{
private:
public:
	static string getHomeDir()
	{

		struct passwd *pw = getpwuid(getuid());

		char *homedir = pw->pw_dir;
		return homedir;
	}
	static bool isDirExist(const std::string &path)
	{
#if defined(_WIN32)
		struct _stat info;
		if (_stat(path.c_str(), &info) != 0)
		{
			return false;
		}
		return (info.st_mode & _S_IFDIR) != 0;
#else
		struct stat info;
		if (stat(path.c_str(), &info) != 0)
		{
			return false;
		}
		return (info.st_mode & S_IFDIR) != 0;
#endif
	}

	static string join(string base, string subPath)
	{
#if defined(_WIN32)
		string fullPath = base + "\\" + subPath;
		return fullPath;
#else
		string fullPath = base + "/" + subPath;
		return fullPath;
#endif
	}

	static bool makePath(const std::string &path)
	{
		if (!isDirExist(path))
		{
#if defined(_WIN32)
			int ret = _mkdir(path.c_str());
#else
			mode_t mode = 0755;
			int ret = mkdir(path.c_str(), mode);
#endif
			if (ret == 0)
				return true;

			switch (errno)
			{
			case ENOENT:
				// parent didn't exist, try to create it
				{
					int pos = path.find_last_of('/');
					if (pos == std::string::npos)
#if defined(_WIN32)
						pos = path.find_last_of('\\');
					if (pos == std::string::npos)
#endif
						return false;
					if (!makePath(path.substr(0, pos)))
						return false;
				}
				// now, try to create again
#if defined(_WIN32)
				return 0 == _mkdir(path.c_str());
#else
				return 0 == mkdir(path.c_str(), mode);
#endif

			case EEXIST:
				// done!
				return isDirExist(path);

			default:
				return false;
			}
		}
		else
			cout << "directory already exists" << endl;
		return true;
	}
};


// implemetnt the callback
void save_FrameArrived(Frame f)
{
    // if ( f.ID >= 10)
    // {
        // string fm = running_profile.GetFormatText();
		string testBasePath1 = FileUtils::join(FileUtils::getHomeDir()+"/Logs", sid);
        string w = to_string(running_profile.resolution.width);
        string h = to_string(running_profile.resolution.height);
        string fps = to_string(running_profile.fps);

        string fileName =  w + "_" + h + "_" + fps + "_" + to_string(f.ID) +".bin";
        string imagePath = File_Utils::join(testBasePath1, fileName);
        write_to_bin_file(imagePath, f.Buff, running_profile.resolution.width * running_profile.resolution.height * running_profile.GetBpp());
    //     isCollectFrames = false;
    // }
}

void AddFrame(Frame frame)
{

	switch (frame.streamType)
	{
	case StreamType::Depth_Stream:
		if (depth_collectFrames)
    {
			depthFramesList.push_back(frame);
      if (isContentTest)
			{
				AnalayzerFrame af;
				af.frame = frame;
				af.fps = currDepthProfile.fps;
				af.pixelFormat = currDepthProfile.GetFormatText();
				af.width = currDepthProfile.resolution.width;
				af.height = currDepthProfile.resolution.height;
				af.size = currDepthProfile.GetSize();
				fa.collect_depth_frame(af);
			}
    }
		break;
	case StreamType::IR_Stream:
		if (ir_collectFrames)
    {
			irFramesList.push_back(frame);
      if (isContentTest)
			{
				AnalayzerFrame af;
				af.frame = frame;
				af.fps = currIRProfile.fps;
				af.pixelFormat = currIRProfile.GetFormatText();
				af.width = currIRProfile.resolution.width;
				af.height = currIRProfile.resolution.height;
				af.size = currIRProfile.GetSize();
				fa.collect_infrared_frame(af);
			}
    }
		break;
	case StreamType::Color_Stream:
		save_FrameArrived(frame);
		if (color_collectFrames)

    {
      colorFramesList.push_back(frame);
      if (isContentTest)
			{
				AnalayzerFrame af;
				af.frame = frame;
				af.fps = currColorProfile.fps;
				af.pixelFormat = currColorProfile.GetFormatText();
				af.width = currColorProfile.resolution.width;
				af.height = currColorProfile.resolution.height;
				af.size = currColorProfile.GetSize();
				fa.collect_color_frame(af);
			}
    }

		break;
		case StreamType::Imu_Stream:
		if (imu_collectFrames)

		{
			if (frame.frameMD.getMetaDataByString("imuType")==1)
				accelFrameList.push_back(frame);
			else if (frame.frameMD.getMetaDataByString("imuType")==2)
				gyroFrameList.push_back(frame);
		}

		break;
	default:
		break;
	}
	// }
}

class MetricDefaultTolerances
{
private:
	static const int tolerance_FirstFrameDelay_high = 1000;
	static const int tolerance_FirstFrameDelay_low = 3000;
	static const int tolerance_SequentialFrameDrops = 2;
	static const int tolerance_FrameDropInterval = 1; // Tolerance is always 1 hard coded - what changes is the interval
	static const int tolerance_FrameDropsPercentage = 5;
	static const int tolerance_FramesArrived = 5;
	static const int tolerance_FpsValidity = 5;
	static const int tolerance_FrameSize = 1;
	static const int tolerance_IDCorrectness = 1; // Tolerance is not used - if one frame has MD error, Metric Fails
	static const int tolerance_ControlLatency = 5;
	static const int tolerance_MetaDataCorrectness = 1; // Tolerance is not used - if one frame has MD error, Metric Fails
	static const int tolerance_CPU = 35;
	static const int tolerance_Memory = 400;
	static const int tolerance_asic_temperature = 100;
	static const int tolerance_projector_temperature = 100;
	static const int tolerance_corrupted = 1;
	static const int tolerance_freeze = 1;

public:
	static int get_tolerance_corrupted()
	{
		return tolerance_corrupted;
	}
	static int get_tolerance_freeze()
	{
		return tolerance_freeze;
	}
	static int get_tolerance_asic_temperature()
	{
		return tolerance_asic_temperature;
	}
	static int get_tolerance_projector_temperature()
	{
		return tolerance_projector_temperature;
	}
	static int get_tolerance_CPU()
	{
		return tolerance_CPU;
	}
	static int get_tolerance_Memory()
	{
		return tolerance_Memory;
	}
	static int get_tolerance_FirstFrameDelay()
	{

		// if fps==0, that means that this profile is not used .
		if (currColorProfile.fps != 0 && currColorProfile.fps != 5 && currColorProfile.fps != 15)
			return tolerance_FirstFrameDelay_high;

		else if (currDepthProfile.fps != 0 && currDepthProfile.fps != 5 && currDepthProfile.fps != 15)
			return tolerance_FirstFrameDelay_high;

		else if (currIRProfile.fps != 0 && currIRProfile.fps != 5 && currIRProfile.fps != 15)
			return tolerance_FirstFrameDelay_high;

		else
			return tolerance_FirstFrameDelay_low;
	}
	static int get_tolerance_FirstFrameDelay(Profile profile)
	{
		if (profile.fps == 5 || profile.fps == 15)
			return tolerance_FirstFrameDelay_low;
		else
			return tolerance_FirstFrameDelay_high;
	}
	static int get_tolerance_SequentialFrameDrops()
	{
		return tolerance_SequentialFrameDrops;
	}
	static int get_tolerance_FrameDropInterval()
	{
		return tolerance_FrameDropInterval;
	}
	static int get_tolerance_FrameDropsPercentage()
	{
		return tolerance_FrameDropsPercentage;
	}
	static int get_tolerance_FramesArrived()
	{
		return tolerance_FramesArrived;
	}
	static int get_tolerance_FpsValidity()
	{
		return tolerance_FpsValidity;
	}
	static int get_tolerance_FrameSize()
	{
		return tolerance_FrameSize;
	}
	static int get_tolerance_IDCorrectness()
	{
		return tolerance_IDCorrectness;
	}
	static int get_tolerance_ControlLatency()
	{
		return tolerance_ControlLatency;
	}
	static int get_tolerance_MetaDataCorrectness()
	{
		return tolerance_MetaDataCorrectness;
	}
};


struct MultiProfile
{
	vector<Profile> profiles;
};

class MetricResult
{
public:
	string remarks;
	bool result;
	string value;
	vector<string> getRemarksStrings()
	{
		vector<string> result;
		istringstream f(remarks);
		string s;
		while (getline(f, s, '\n'))
		{
			result.push_back(s);
		}
		return result;
	}
};

class PNPMetricResult
{
public:
	string remarks;
	bool result;
	float min, max, average, tolerance;
	string value;
	vector<string> getRemarksStrings()
	{
		vector<string> result;
		istringstream f(remarks);
		string s;
		while (getline(f, s, '\n'))
		{
			result.push_back(s);
		}
		return result;
	}
};

class ContentMetric
{
public:
	int _tolerance;
	Profile _profile;
	string _metricName;
	vector<CorruptedResult> _corrupted_results;
	vector<FreezeResult> _freeze_results;
	bool _useSystemTs = false;
	virtual MetricResult calc() = 0;
	void configure(Profile profile, vector<CorruptedResult> corrupted_results, vector<FreezeResult> freeze_results)
	{
		_profile = profile;
		_corrupted_results = corrupted_results;
		_freeze_results = freeze_results;
	}
};

class CorruptedMetric : public ContentMetric
{
public:
	CorruptedMetric()
	{
		_metricName = "Corrupted Frames";
	}
	void SetCorruptedResults(vector<CorruptedResult> r)
	{
		_corrupted_results = r;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	MetricResult calc()
	{
		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");

		MetricResult r;
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		if (_corrupted_results.size() == 0)
		{
			// throw std::runtime_error("Corrupted results array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}
			
		bool is_first_corrupted = true;
		int first_corrupted_index = -1;

		int corrupted_count = 0;
		int high_or_low_images_count = 0;
		for (int i = 0; i < _corrupted_results.size(); i++)
		{
			if (_corrupted_results[i].is_corrupted)
			{
				if (is_first_corrupted)
				{
					first_corrupted_index = _corrupted_results[i].frame_id;
					is_first_corrupted = false;
				}
				corrupted_count++;
			}
			if (_corrupted_results[i].is_high_or_low_image_pixels){
				high_or_low_images_count++;
			}
		}
		if (corrupted_count >= _tolerance)
			r.result = false;
		else
			r.result = true;
		r.remarks = "Corrupted frames count: " + to_string(corrupted_count) + "\nDark/ Saturated frames count: " + to_string(high_or_low_images_count) + "\nTotal analyzed frames: " + to_string(_corrupted_results.size()) + "\nFirst corrupted frame index: " + to_string(first_corrupted_index) +
					"\nTolerance: " + to_string(_tolerance) + " corrupted frame or more\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
    r.value = to_string(corrupted_count);
		return r;
	}
};

class FreezeMetric : public ContentMetric
{
public:
	FreezeMetric()
	{
		_metricName = "Freeze Frames";
	}
	void SetCorruptedResults(vector<FreezeResult> r)
	{
		_freeze_results = r;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	MetricResult calc()
	{
		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");
		MetricResult r;

		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		if (_freeze_results.size() == 0)
		{
			// throw std::runtime_error("Freeze results array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}

		int freeze_count = 0;
		int first_seq_index = -1;
		int last_seq_index = -1;
		int max_sequential = 0;
		int max_sequential_index = -1;
		int first_freeze_index = -1;
		for (int i = 0; i < _freeze_results.size(); i++)
		{
			string status;
			FreezeResult fr = _freeze_results[i];
			if (fr.is_freeze)
			{
				if (first_freeze_index == -1)
				{
					first_freeze_index = fr.first_frame_id;
				}
				if (first_seq_index == -1)
				{
					first_seq_index = fr.first_frame_id;
				}
				last_seq_index = fr.second_frame_id;
				if (last_seq_index - first_seq_index > max_sequential)
				{
					max_sequential = last_seq_index - first_seq_index;
					max_sequential_index = first_seq_index;
				}
				freeze_count++;
			}
			else
			{
				first_seq_index = -1;
				last_seq_index = -1;
			}
		}
		if (freeze_count >= _tolerance)
			r.result = false;
		else
			r.result = true;
		r.remarks = "Freeze events count: " + to_string(freeze_count) + "\nTotal analyzed frames: " + to_string(_freeze_results.size()) + "\nFirst freeze frame index: " + to_string(first_freeze_index) +
					"\nMax sequential freeze: " + to_string(max_sequential) + "\nMax sequential freeze index: " + to_string(max_sequential_index) + "\nTolerance: " + to_string(_tolerance) + " freeze event or more\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
    r.value = to_string(freeze_count);
		return r;
	}
};

class PnpMetric
{
public:
	int _tolerance;
	string _metricName;
	vector<float> _samples;
	virtual PNPMetricResult calc() = 0;
	void configure(vector<float> samples)
	{
		_samples = samples;
	}
};

class CPUMetric : public PnpMetric
{
public:
	CPUMetric()
	{
		_metricName = "CPU Consumption";
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	PNPMetricResult calc()
	{
		float min = 100;
		float max = 0;
		float sum = 0;
		float average;
		if (_samples.size() == 0)
		{
			throw std::runtime_error("No PNP samples to calculate");
		}
		for (int i = 0; i < _samples.size(); i++)
		{
			if (_samples[i] < min)
				min = _samples[i];
			if (_samples[i] >= max)
				max = _samples[i];
			sum += _samples[i];
		}
		average = sum / _samples.size();

		PNPMetricResult r;
		if (average >= _tolerance)
			r.result = false;
		else
			r.result = true;
		r.average = average;
		r.min = min;
		r.max = max;
		r.tolerance = _tolerance;
		r.remarks = "Average CPU consumption: " + to_string(average) + "%\nMin CPU consumption: " + to_string(min) + "%\nMax CPU consumption: " + to_string(max) + "%\nTolerance: " + to_string(_tolerance) + "%\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(average);
		return r;
	}
};

class MemMetric : public PnpMetric
{
public:
	MemMetric()
	{
		_metricName = "Memory Consumption";
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	PNPMetricResult calc()
	{
		float min = -1;
		float max = 0;
		float sum = 0;
		float average;
		if (_samples.size() == 0)
		{
			throw std::runtime_error("No PNP samples to calculate");
		}
		for (int i = 0; i < _samples.size(); i++)
		{
			if (_samples[i] < min || min == -1)
				min = _samples[i];
			if (_samples[i] >= max)
				max = _samples[i];
			sum += _samples[i];
		}
		average = sum / _samples.size();

		PNPMetricResult r;
		if (average >= _tolerance)
			r.result = false;
		else
			r.result = true;
		r.average = average;
		r.min = min;
		r.max = max;
		r.tolerance = _tolerance;
		r.remarks = "Average Memory consumption: " + to_string(average) + "MB\nMin Memory consumption: " + to_string(min) + "MB\nMax Memory consumption: " + to_string(max) + "MB\nTolerance: " + to_string(_tolerance) + "MB\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(average);
		return r;
	}
};

class AsicTempMetric : public PnpMetric
{
public:
	int sample = -1;
	AsicTempMetric()
	{
		_metricName = "Asic Temperature";
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	PNPMetricResult calc()
	{
		float min = -1;
		float max = 0;
		float sum = 0;
		float average;
		if (_samples.size() == 0)
		{
			throw std::runtime_error("No ASIC Temperature samples to calculate");
		}
		for (int i = 0; i < _samples.size(); i++)
		{
			if (_samples[i] < min || min == -1)
				min = _samples[i];
			if (_samples[i] >= max)
				max = _samples[i];
			sum += _samples[i];
		}
		average = sum / _samples.size();

		PNPMetricResult r;
		if (average >= _tolerance)
			r.result = false;
		else
			r.result = true;
		r.average = average;
		r.min = min;
		r.max = max;
		r.tolerance = _tolerance;
		r.remarks = "Average Asic temperature: " + to_string(average) + "\nMin Asic temperature: " + to_string(min) + "\nMax Asic temperature: " + to_string(max) + "\nTolerance: " + to_string(_tolerance) + "\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(average);
		return r;
	}
};

class ProjTempMetric : public PnpMetric
{
public:
	ProjTempMetric()
	{
		_metricName = "Projector Temperature";
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	PNPMetricResult calc()
	{
		float min = -1;
		float max = 0;
		float sum = 0;
		float average;
		if (_samples.size() == 0)
		{
			throw std::runtime_error("No Projector Temperature samples to calculate");
		}
		for (int i = 0; i < _samples.size(); i++)
		{
			if (_samples[i] < min || min == -1)
				min = _samples[i];
			if (_samples[i] >= max)
				max = _samples[i];
			sum += _samples[i];
		}
		average = sum / _samples.size();

		PNPMetricResult r;
		if (average >= _tolerance)
			r.result = false;
		else
			r.result = true;
		r.average = average;
		r.min = min;
		r.max = max;
		r.tolerance = _tolerance;
		r.remarks = "Average Proj temperature: " + to_string(average) + "\nMin Proj temperature: " + to_string(min) + "\nMax Proj temperature: " + to_string(max) + "\nTolerance: " + to_string(_tolerance) + "\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(average);
		return r;
	}
};

class Metric
{
public:
	int _tolerance;
	Profile _profile;
	string _metricName;
	vector<Frame> _frames;
	bool _useSystemTs = false;
	virtual MetricResult calc() = 0;
	void configure(Profile profile, vector<Frame> frames)
	{
		_profile = profile;
		_frames = frames;
	}
	int getFPSByExposure(int exposure)
	{
		int fps;
		if (_profile.streamType == StreamType::Depth_Stream || _profile.streamType == StreamType::IR_Stream)
		{
			if (exposure >= 1 && exposure <= 10000)
				fps = 90;
			else if (exposure >= 10020 && exposure <= 15500)
				fps = 60;
			else if (exposure >= 15520 && exposure <= 32200)
				fps = 30;
			else if (exposure >= 32220 && exposure <= 65500)
				fps = 15;
			else if (exposure >= 65520) //&& exposure <= 165000)
				fps = 5;
		}
		else if (_profile.streamType == StreamType::Color_Stream)
		{
			fps = 10000 / exposure;
		}
		if (fps < _profile.fps)
			return fps;
		else
			return _profile.fps;
	}
	double CalcMedian(vector<double> deltas)
	{
		size_t size = deltas.size();

		if (size == 0)
		{
			return 0;  // Undefined, really.
		}
		else
		{
			sort(deltas.begin(), deltas.end());
			if (size % 2 == 0)
			{
				return (deltas[size / 2 - 1] + deltas[size / 2]) / 2;
			}
			else
			{
				return deltas[size / 2];
			}
		}
	}

	double getActualFPS(vector<Frame> frames, double fps, bool useSystemTS, int indexOfChange, bool fromIdexOfChangeAndUp = true)
	{
		//int numberOfFrames = 0;
		//int sumOfDeltas = 0;
		double actualFPS;
		double delta;
		double expectedDelta = 1000 / fps;
		vector<double> deltas;
		if (fromIdexOfChangeAndUp)
		{
			for (int i = indexOfChange; i < frames.size(); i++)
			{
				if (useSystemTS)
				{
					deltas.push_back(frames[i].systemTimestamp - frames[i - 1].systemTimestamp);
				}
				else
				{
					deltas.push_back((frames[i].frameMD.getMetaDataByString("Timestamp") - frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0);
				}
			}
		}
		else
		{
			for (int i = 5; i < indexOfChange; i++)
			{
				if (useSystemTS)
				{
					deltas.push_back(frames[i].systemTimestamp - frames[i - 1].systemTimestamp);
				}
				else
				{
					deltas.push_back((frames[i].frameMD.getMetaDataByString("Timestamp") - frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0);
				}
			}
		}

		delta= CalcMedian(deltas);
		// go over all frame deltas and calc the fps  - dont count the frame drops
		/*
		if (skipDrops)
		{
			for (int i = 5; i < frames.size(); i++)
			{
				if (useSystemTS)
				{
					delta = frames[i].systemTimestamp - frames[i - 1].systemTimestamp;
				}
				else
				{
					delta = (frames[i].frameMD.getMetaDataByString("Timestamp") - frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
				}
				if (delta <= 1.5 * expectedDelta && delta >= 0.5 * expectedDelta)
				{
					numberOfFrames += 1;
					sumOfDeltas += delta;
				}
			}
		}
		if (numberOfFrames <= 5)
		{
			// if there are no frames - all frames have drops, then calculate the fps including frame drops
			sumOfDeltas = 0;
			numberOfFrames = 0;
			for (int i = 1; i < frames.size(); i++)
			{
				if (useSystemTS)
				{
					delta = frames[i].systemTimestamp - frames[i - 1].systemTimestamp;
				}
				else
				{
					delta = (frames[i].frameMD.getMetaDataByString("Timestamp") - frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
				}
				if (delta>=0)
				{
				numberOfFrames += 1;
				sumOfDeltas += delta;
				}
			}
		}*/

		actualFPS = 1000.0 / delta;
		Logger::getLogger().log("Median delta: " + to_string(delta) + ",Actual FPS : " + to_string(actualFPS), "Metric");
		return actualFPS;
	}
};

class FirstFrameDelayMetric : public Metric
{
private:
	long _startTime = 0;

public:
	FirstFrameDelayMetric()
	{
		_metricName = "First frame delay";
	}
	void SetStartTime(double start)
	{
		_startTime = start;
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance, long startTime)
	{
		_tolerance = tolerance;
		_startTime = startTime;
	}
	MetricResult calc()
	{
		_tolerance = MetricDefaultTolerances::get_tolerance_FirstFrameDelay(_profile);
		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");
		
		if (_startTime == 0)
			throw std::runtime_error("Missing Start time in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric",LOG_ERROR);
			r.value = "0";
			return r;
		}
		double firstFrameDelay = _frames[0].systemTimestamp - _startTime;
		//if (_profile.streamType == StreamType::IR_Stream)
		//{
		//	if (currColorProfile.fps != 0)
		//		firstFrameDelay -= 1000;
		//	if (currDepthProfile.fps != 0)
		//		firstFrameDelay -= 1000;
		//}
		//if (_profile.streamType == StreamType::Depth_Stream)
		//{
		//	if (currColorProfile.fps != 0)
		//		firstFrameDelay -= 1000;
		//}
		if (firstFrameDelay >= _tolerance)
			r.result = false;
		else
			r.result = true;

		r.remarks = "First frame delay :" + to_string(firstFrameDelay) + "ms \nTolerance: " + to_string(_tolerance) + "ms \nIteration result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(firstFrameDelay);
		return r;
	}
};

class SequentialFrameDropsMetric : public Metric
{
private:
	bool _autoExposureOff = false;
	int _currExp;
	double _changeTime;
	double _value;
	string _metaDataName;

public:
	SequentialFrameDropsMetric()
	{
		_metricName = "Sequential frame drops";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	void setParams(int tolerance, int currExp, double changeTime, string metaDataName, double value)
	{
		_tolerance = tolerance;
		_autoExposureOff = true;
		_currExp = currExp;
		_changeTime = changeTime;
		_value = value;
		_metaDataName = metaDataName;
	}
	MetricResult calc()
	{
		double actualDelta;
		int droppedFrames = 0;
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}

		string text = "";
		int indexOfChange;
		double fps;
		if (_autoExposureOff)
		{
			indexOfChange = -1;
			for (int i = 0; i < _frames.size(); i++)
			{
				if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
				{
					indexOfChange = i+1;

					break;
				}
			}
			fps = getFPSByExposure(_currExp);
		}
		else
		{
			fps = _profile.fps;
			indexOfChange = 5;
		}
		fps = getActualFPS(_frames, fps, _useSystemTs, indexOfChange);
		double expectedDelta = 1000.0 / fps;
		int minDropIndex = 0, minDrops = 0, maxDropIndex = 0, maxDrops = 0, totalFramesDropped = 0, totalSequentialFramesDropped = 0, sequentialFrameDropEvents = 0, firstSequentialDropIndex = 0;
		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");
		for (int i = indexOfChange; i < _frames.size(); i++)
		{
			if (_useSystemTs)
			{
				actualDelta = (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
				text = "Used System Timestamp to calculate this metric\n";
			}
			else
			{
				actualDelta = (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
				if (actualDelta < 0)
					continue;
			}
			droppedFrames = round(actualDelta / expectedDelta) - 1;
			double previousDelta;
			for (int j = droppedFrames; j > 0; j--)
			{
				if (i - j - 1 >= 0)
				{
					if (_useSystemTs)
					{
						previousDelta = (_frames[i - j].systemTimestamp - _frames[i - j - 1].systemTimestamp);
					}
					else
					{
						previousDelta = (_frames[i - j].frameMD.getMetaDataByString("Timestamp") - _frames[i - j - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
					}
					if (previousDelta == 0)
					{
						droppedFrames--;
						actualDelta = -expectedDelta;
					}
				}
			}
			if (droppedFrames > 0)
				totalFramesDropped += droppedFrames;
			if (actualDelta >= 2.5 * expectedDelta)
			{
				totalSequentialFramesDropped += droppedFrames;
				sequentialFrameDropEvents++;
				if (droppedFrames > maxDrops)
				{
					maxDropIndex = _frames[i].ID;
					maxDrops = droppedFrames;
				}
				if (droppedFrames < minDrops || minDrops==0)
				{
					minDrops = droppedFrames;
					minDropIndex = _frames[i].ID;
				}
				if (firstSequentialDropIndex == 0)
					firstSequentialDropIndex = _frames[i].ID;
			}
		}
		if (maxDrops >= _tolerance)
			r.result = false;
		else
			r.result = true;

		r.remarks = text + "First Sequential drop index: " + to_string(firstSequentialDropIndex) + "\nSequential frame Drop Events: " + to_string(sequentialFrameDropEvents) + "\nTotal frames dropped: " + to_string(totalFramesDropped) + "\nPercentage of sequential drops out of all drops: " + to_string(100.0*totalSequentialFramesDropped/totalFramesDropped) +
			"\nMax sequential frame drops: " + to_string(maxDrops) + "\nMax sequential drop index: " + to_string(maxDropIndex) + "\nMin sequential frame drops: " + to_string(minDrops) + "\nMin sequential drop index: " + to_string(minDropIndex) +
			"\nTolerance: " + to_string(_tolerance) + "\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(maxDrops);
		return r;
	}
};

class FrameDropIntervalMetric : public Metric
{
private:
	int _interval;
	bool _autoExposureOff = false;
	int _currExp;
	double _changeTime;
	double _value;
	string _metaDataName;

public:
	FrameDropIntervalMetric()
	{
		_metricName = "Frame drops interval";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int interval)
	{
		_interval = interval;
	}
	void setParams(int interval, int currExp, double changeTime, string metaDataName, double value)
	{
		_interval = interval;
		_autoExposureOff = true;
		_currExp = currExp;
		_changeTime = changeTime;
		_value = value;
		_metaDataName = metaDataName;
	}
	MetricResult calc()
	{
		int eventCount = 0;
		double lastDropTS = 0;
		double currTS;
		double actualDelta;
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}

		string text = "";
		bool status = true;
		int firstFail = -1;
		double expectedDelta;
		int droppedFrames;
		double previousDelta;
		double actualFPS;
		if (!_autoExposureOff)
		{
			actualFPS = getActualFPS(_frames, _profile.fps, _useSystemTs,5);
			expectedDelta = 1000.0 / actualFPS;
			Logger::getLogger().log("Calculating metric: " + _metricName + " with interval: " + to_string(_interval) + " on " + _profile.GetText(), "Metric");
			for (int i = 5; i < _frames.size(); i++)
			{
				if (_useSystemTs)
				{
					actualDelta = (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
					text = "Used System Timestamp to calculate this metric\n";
					currTS = _frames[i].systemTimestamp;
				}
				else
				{
					actualDelta = (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
					if (actualDelta < 0)
						continue;
					currTS = _frames[i].frameMD.getMetaDataByString("Timestamp");
				}
				droppedFrames = round(actualDelta / expectedDelta) - 1;
				previousDelta;
				for (int j = droppedFrames; j > 0; j--)
				{
					if (i - j - 1 >= 0)
					{
						if (_useSystemTs)
						{
							previousDelta = (_frames[i - j].systemTimestamp - _frames[i - j - 1].systemTimestamp);
						}
						else
						{
							previousDelta = (_frames[i - j].frameMD.getMetaDataByString("Timestamp") - _frames[i - j - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
						}
						if (previousDelta == 0)
						{
							droppedFrames--;
							actualDelta = -expectedDelta;
						}
					}
				}
				if (actualDelta > 2.5 * expectedDelta)
				{
					status = false;
					eventCount += 1;
					if (firstFail == -1)
						firstFail = _frames[i].ID;
					
				}

				else if (actualDelta > 1.5 * expectedDelta)
				{
					if (lastDropTS == 0)

						lastDropTS = currTS;
					else
					{
						if (currTS - lastDropTS < _interval * 1000)
						{
							status = false;
							eventCount += 1;
							if (firstFail==-1)
								firstFail = _frames[i].ID;
							
						}
						else
							lastDropTS = currTS;
					}
				}
			}
		}
		else
		{
			int indexOfChange = -1;
			for (int i = 0; i < _frames.size(); i++)
			{
				if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
				{
					indexOfChange = i+1;

					break;
				}
			}
			actualFPS = getActualFPS(_frames, getFPSByExposure(_currExp), _useSystemTs, indexOfChange);
			expectedDelta = 1000.0 / actualFPS;
			Logger::getLogger().log("Calculating metric: " + _metricName + " with interval: " + to_string(_interval) + " on " + _profile.GetText(), "Metric");
			for (int i = indexOfChange; i < _frames.size(); i++)
			{
				if (_useSystemTs)
				{
					actualDelta = (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
					text = "Used System Timestamp to calculate this metric\n";
					currTS = _frames[i].systemTimestamp;
				}
				else
				{
					actualDelta = (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
					if (actualDelta < 0)
						continue;
					currTS = _frames[i].frameMD.getMetaDataByString("Timestamp");
				}
				droppedFrames = round(actualDelta / expectedDelta) - 1;
				previousDelta;
				for (int j = droppedFrames; j > 0; j--)
				{
					if (i - j - 1 >= 0)
					{
						if (_useSystemTs)
						{
							previousDelta = (_frames[i - j].systemTimestamp - _frames[i - j - 1].systemTimestamp);
						}
						else
						{
							previousDelta = (_frames[i - j].frameMD.getMetaDataByString("Timestamp") - _frames[i - j - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
						}
						if (previousDelta == 0)
						{
							droppedFrames--;
							actualDelta = -expectedDelta;
						}
					}
				}
				if (actualDelta > 2.5 * expectedDelta)
				{
					status = false;
					eventCount += 1;
					if (firstFail == -1)
						firstFail = _frames[i].ID;
				}

				else if (actualDelta > 1.5 * expectedDelta)
				{
					if (lastDropTS == 0)

						lastDropTS = currTS;
					else
					{
						if (currTS - lastDropTS < _interval * 1000)
						{
							status = false;
							eventCount += 1;
							if (firstFail == -1)
								firstFail = _frames[i].ID;
						}
						else
							lastDropTS = currTS;
					}
				}
			}
		}
		r.result = status;

		r.remarks = text + "Interval Drop Event Count: "+ to_string(eventCount)+"\nFirst fail index : " + to_string(firstFail) + "\nInterval : " + to_string(_interval) + "\nMetric result : " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(eventCount);
		return r;
	}
};

class FrameDropsPercentageMetric : public Metric
{
private:
	bool _autoExposureOff = false;
	int _currExp;
	double _changeTime;
	double _value;
	string _metaDataName;

public:
	FrameDropsPercentageMetric()
	{
		_metricName = "Frame drops percentage";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	void setParams(int tolerance, int currExp, double changeTime, string metaDataName, double value)
	{
		_tolerance = tolerance;
		_autoExposureOff = true;
		_currExp = currExp;
		_changeTime = changeTime;
		_value = value;
		_metaDataName = metaDataName;
	}
	MetricResult calc()
	{
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}
		string text = "";
		double actualDelta = 0;
		int indexOfChange;
		double fps;
		if (_autoExposureOff)
		{
			indexOfChange = -1;
			for (int i = 0; i < _frames.size(); i++)
			{
				if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
				{
					indexOfChange = i+1;

					break;
				}
			}
			fps = getFPSByExposure(_currExp);
		}
		else
		{
			fps = _profile.fps;
			indexOfChange = 5;
		}
		fps = getActualFPS(_frames, fps, _useSystemTs, indexOfChange);
		double expectedDelta = 1000 / fps;
		int droppedFrames = 0;
		int totalFramesDropped = 0;
		int expectedFrames;
		if (_useSystemTs)
			expectedFrames = fps * (_frames[_frames.size() - 1].systemTimestamp - _frames[indexOfChange].systemTimestamp) / 1000;
		else
			expectedFrames = fps * (_frames[_frames.size() - 1].frameMD.getMetaDataByString("Timestamp") - _frames[indexOfChange].frameMD.getMetaDataByString("Timestamp")) / 1000 / 1000;
		// Calculate # of frames dropped
		for (int i = indexOfChange; i < _frames.size(); i++)
		{
			if (_useSystemTs)
			{
				actualDelta = (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
				text = "Used System Timestamp to calculate this metric\n";
			}
			else
			{
				actualDelta = (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
				if (actualDelta < 0)
					continue;
			}

			droppedFrames = (actualDelta / expectedDelta) - 1;
			if (droppedFrames > 0)
			{
				double previousDelta;
				for (int j = droppedFrames; j > 0; j--)
				{
					if (i - j - 1 >= 0)
					{
						if (_useSystemTs)
						{
							previousDelta = (_frames[i - j].systemTimestamp - _frames[i - j - 1].systemTimestamp);
						}
						else
						{
							previousDelta = (_frames[i - j].frameMD.getMetaDataByString("Timestamp") - _frames[i - j - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
						}
						if (previousDelta == 0)
						{
							droppedFrames--;
							actualDelta = -expectedDelta;
						}
					}
				}
			}

			if (droppedFrames > 0)
				totalFramesDropped += droppedFrames;
		}

		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + "% on " + _profile.GetText(), "Metric");

		if (100.0 * totalFramesDropped / expectedFrames >= _tolerance)
			r.result = false;
		else
			r.result = true;

		r.remarks = text + "Total frames dropped: " + to_string(totalFramesDropped) + "\nExpected Frames: " + to_string(expectedFrames) + "\nDrop percentage: " + to_string(100.0 * totalFramesDropped / expectedFrames) + "%\nTolerance: " + to_string(_tolerance) + "%\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(100.0 * totalFramesDropped / expectedFrames);
		return r;
	}
};

class FramesArrivedMetric : public Metric
{
private:
	long _startTime = 0;
	int _testDuration = 0;

public:
	FramesArrivedMetric()
	{
		_metricName = "Frames arrived";
	}
	void SetStartTime(double start)
	{
		_startTime = start;
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance, long startTime, int testDuration)
	{
		_tolerance = tolerance;
		_startTime = startTime;
		_testDuration = testDuration;
	}
	MetricResult calc()
	{
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}
		double actualStreamDuration = _testDuration * 1000;
		double ttff = _frames[0].systemTimestamp - _startTime;
		int zero_delta_frames = 0, droppedFrames =0 , totalFramesDropped = 0;
		//if (_profile.streamType == StreamType::IR_Stream)
		//{
		//	if (currColorProfile.fps != 0)
		//		ttff -= 1000;
		//	if (currDepthProfile.fps != 0)
		//		ttff -= 1000;
		//}
		//if (_profile.streamType == StreamType::Depth_Stream)
		//{
		//	if (currColorProfile.fps != 0)
		//		ttff -= 1000;
		//}
		// int droppedFrames, totalFramesDropped = 0;
		string text = "";
		double actualDelta;
		double fps = getActualFPS(_frames, _profile.fps, _useSystemTs, 5);
		double expectedDelta = 1000.0 / fps;
		// Calculate # of frames dropped
		for (int i = 5; i < _frames.size(); i++)
		{
			if (_useSystemTs)
			{
				actualDelta = (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
				text = "Used System Timestamp to calculate this metric\n";
			}
			else
			{
				actualDelta = (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
				if (actualDelta < 0)
					continue;
			}
			if (actualDelta == 0)
			{
				zero_delta_frames += 1;
			}
			// TODO round(actualDelta / expectedDelta)
			droppedFrames = round(actualDelta / expectedDelta) - 1;

			if (droppedFrames > 0)
				totalFramesDropped += droppedFrames;
		}

		double expectedFrames = ceil(((actualStreamDuration - ttff) / 1000) * fps - totalFramesDropped + zero_delta_frames);
		double actualFramesArrived = _frames.size();
		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");

		double percentage = (1 - (actualFramesArrived / expectedFrames)) * 100;
		if (percentage >= _tolerance)
			r.result = false;
		else
			r.result = true;
		r.remarks = text + "Actual Stream Duration :" + to_string(actualStreamDuration / 1000) + "seconds \nTime to first frame: " + to_string(ttff) +
					"\nFPS : " + to_string(_profile.fps) + "\nActual Frames Arrived#:" + to_string(actualFramesArrived) + "\nExpected Frames: " + to_string(expectedFrames) +
					"\nDropped Frames: " + to_string(totalFramesDropped)+"\nZero Delta frames: "+to_string(zero_delta_frames) + "\nTolerance: +-" + to_string(_tolerance) + "%\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string((actualFramesArrived / expectedFrames) * 100);
		return r;
	}
};

class FpsValidityMetric : public Metric
{
public:
	bool _autoExposureOff = false;
	int _currExp;
	double _changeTime;
	double _value;
	string _metaDataName;
	FpsValidityMetric()
	{
		_metricName = "FPS Validity";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	void setParams(int tolerance, int currExp, double changeTime, string metaDataName, double value)
	{
		_tolerance = tolerance;
		_autoExposureOff = true;
		_currExp = currExp;
		_changeTime = changeTime;
		_value = value;
		_metaDataName = metaDataName;
	}
	MetricResult calc()
	{
		double sumOfDeltas = 0;
		double averageDelta;
		double expectedDelta;
		double fps;
		double expectedFps;
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}
		string text = "";
		if (_useSystemTs)
				{
					text = "Used System Timestamp to calculate this metric\n";
				}
		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");
		/*
		int skippedFrames = 5;
		
		
		double percentage;
		if (!_autoExposureOff)
		{
			expectedDelta = 1000.0 / _profile.fps;
			Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");
			for (int i = 5; i < _frames.size(); i++)
			{
				if (_useSystemTs)
				{
					sumOfDeltas += (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
					text = "Used System Timestamp to calculate this metric\n";
				}
				else
				{
					if ((_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) < 0)
					{
						skippedFrames++;
						continue;
					}
					sumOfDeltas += (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
				}
			}
			averageDelta = sumOfDeltas / (_frames.size() - skippedFrames);

			percentage = abs((1 - (averageDelta / expectedDelta)) * 100);
			if (percentage >= _tolerance)
				r.result = false;
			else
				r.result = true;
		}
		else
		{
			int indexOfChange = -1;
			for (int i = 0; i < _frames.size(); i++)
			{
				if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
				{
					indexOfChange = i+1;

					break;
				}
			}
			double actualFPS = getFPSByExposure(_currExp);
			expectedDelta = 1000.0 / actualFPS;
			Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");
			for (int i = indexOfChange ; i < _frames.size(); i++)
			{
				if (_useSystemTs)
				{
					sumOfDeltas += (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
					text = "Used System Timestamp to calculate this metric\n";
				}
				else
				{
					if ((_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) < 0)
					{
						skippedFrames++;
						continue;
					}
					sumOfDeltas += (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
				}
			}
			*/
			int indexOfChange;
			if (_autoExposureOff)
			{
				expectedFps = getFPSByExposure(_currExp);
				
				indexOfChange = -1;
				for (int i = 0; i < _frames.size(); i++)
				{
					if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
					{
						indexOfChange = i + 1;

						break;
					}
				}
				
			}
			else
			{
				expectedFps = _profile.fps;
				// check if Gyro stream and FPS is 50, set FPS to 100
				if (_profile.fps == 50 && _frames[5].frameMD.getMetaDataByString("imuType")==2)
					{
						expectedFps = 100;
					}
				indexOfChange = 5;
			}
			// expected delta according to the FPS before calculating the actual FPS
			expectedDelta = 1000.0 / expectedFps;

			fps = getActualFPS(_frames, expectedFps, _useSystemTs, indexOfChange);
			
			//averageDelta = sumOfDeltas / (_frames.size()  - indexOfChange);
			averageDelta = 1000 / fps;
			double percentage = abs((1 - (averageDelta / expectedDelta)) * 100);
			if (percentage >= _tolerance)
				r.result = false;
			else
				r.result = true;
		// }

			r.remarks = text + "Average Delta: " + to_string(averageDelta)+"\nExpected delta: "+to_string(expectedDelta)+"\nAverage FPS : " + to_string(fps) + "\nExpected FPS : " + to_string(expectedFps) +
					"\nTolerance: " + to_string(_tolerance) + "\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(averageDelta);
		return r;
	}
};

class FrameSizeMetric : public Metric
{
public:
	FrameSizeMetric()
	{
		_metricName = "Frame Size";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	MetricResult calc()
	{
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}
		int numberOfcorruptFrames = 0;
		double expectedFrameSize = _profile.GetSize();
		Logger::getLogger().log("Calculating metric: " + _metricName + " with Tolerance: " + to_string(_tolerance) + " on " + _profile.GetText(), "Metric");
		bool once = false;
		for (int i = 5; i < _frames.size(); i++)
		{

			if (_frames[i].size != expectedFrameSize)
			{
				numberOfcorruptFrames++;
				if (once == false)
				{
					Logger::getLogger().log("Frame Size: " + to_string(_frames[i].size) + " || Expected Frame size: " + to_string(expectedFrameSize));
					once = true;
				}
			}
		}
		double percentage = ((numberOfcorruptFrames / (_frames.size()-5))) * 100.0;
		if (percentage >= _tolerance)
			r.result = false;
		else
			r.result = true;

		r.remarks = "Number of corrupted frames: " + to_string(numberOfcorruptFrames) + "\nNumber of frames Arrived: " + to_string(_frames.size()) +
					"\nTolerance: " + to_string(_tolerance) + "%\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(numberOfcorruptFrames);
		return r;
	}
};

class IDCorrectnessMetric : public Metric
{
private:
	bool _autoExposureOff = false;
	int _currExp;
	double _changeTime;
	double _value;
	string _metaDataName;

public:
	IDCorrectnessMetric()
	{
		_metricName = "ID Correctness";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	void setParams(int tolerance, int currExp, double changeTime, string metaDataName, double value)
	{
		_tolerance = tolerance;
		_autoExposureOff = true;
		_currExp = currExp;
		_changeTime = changeTime;
		_value = value;
		_metaDataName = metaDataName;
	}
	MetricResult calc()
	{
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}

		int indexOfChange;
		double fps;
		if (_autoExposureOff)
		{
			indexOfChange = -1;
			for (int i = 0; i < _frames.size(); i++)
			{
				if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
				{
					indexOfChange = i+1;

					break;
				}
			}
			fps = getFPSByExposure(_currExp);
		}
		else
		{
			fps = _profile.fps;
			indexOfChange = 5;
		}
		fps = getActualFPS(_frames, fps, _useSystemTs,indexOfChange);
		int numberOfReset = 0;
		int numberOfduplicate = 0;
		int numberOfNegative = 0;
		int numberOfInvalid = 0;
		bool status = true;
		double expectedDelta = 1000 / fps;
		double actualDelta = 0;
		int droppedFrames = 0;
		int indexOfFirstFail = -1;
		string text = "";
		Logger::getLogger().log("Calculating metric: " + _metricName + " on " + _profile.GetText(), "Metric");
		for (int i = indexOfChange; i < _frames.size(); i++)
		{
			int idDelta = _frames[i].ID - _frames[i - 1].ID;
			if (_useSystemTs)
			{
				actualDelta = (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
				text = "Used System Timestamp to calculate this metric\n";
			}
			else
			{
				actualDelta = (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
			}
			if (actualDelta < 0)
				droppedFrames = 0;
			else
				droppedFrames = round(actualDelta / expectedDelta) - 1;

			if (_frames[i].ID == 0)
			{
				status = false;
				numberOfReset++;
			}
			else if (idDelta == 0)
			{
				status = false;
				numberOfduplicate++;
			}
			else if (idDelta < 0)
			{
				status = false;
				numberOfNegative++;
			}
			else if (droppedFrames != idDelta - 1)
			{
				if (actualDelta == 0)
					continue;
				int previousDelta;
				for (int j = droppedFrames; j > 0; j--)
				{
					if (i - j - 1 >= 0)
					{
						if (_useSystemTs)
						{
							previousDelta = (_frames[i - j].systemTimestamp - _frames[i - j - 1].systemTimestamp);
						}
						else
						{
							previousDelta = (_frames[i - j].frameMD.getMetaDataByString("Timestamp") - _frames[i - j - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
						}
						if (previousDelta == 0)
						{
							droppedFrames--;
							actualDelta = -expectedDelta;
						}
					}
				}
				if (droppedFrames != idDelta - 1)
				{
					status = false;
					numberOfInvalid++;
				}
			}
			if (status == false && indexOfFirstFail == -1)
				indexOfFirstFail = _frames[i].ID;
		}

		r.result = status;

		r.remarks = text + "Index of first fail: " + to_string(indexOfFirstFail) + "\nNumber of reset counter events: " + to_string(numberOfReset) +
					"\nNumber of duplicate index events: " + to_string(numberOfduplicate) + "\nNumber of negative index delta events: " + to_string(numberOfNegative) +
					"\nNumber of invalid index delta events: " + to_string(numberOfInvalid) + "\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(numberOfReset + numberOfduplicate + numberOfNegative + numberOfInvalid);
		return r;
	}
};

class ControlLatencyMetric : public Metric
{
private:
	bool _autoExposureOff = false;
	double _changeTime;
	double _value;
	double _prev_exposure;
	string _metaDataName;

public:
	ControlLatencyMetric()
	{
		_metricName = "Control latency";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance, double changeTime, string metaDataName, double value)
	{
		_tolerance = tolerance;
		_changeTime = changeTime;
		_metaDataName = metaDataName;
		// if (_metaDataName == "exposureTime")
			// value = value * 100;
		_value = value;
	}
	void setParams(int tolerance, double changeTime, string metaDataName, double value, double prev_exposure)
	{
		_tolerance = tolerance;
		_changeTime = changeTime;
		_metaDataName = metaDataName;
		// if (_metaDataName == "exposureTime")
		// 	value = value * 100;
		_value = value;
		_prev_exposure = prev_exposure * 100;
		_autoExposureOff = true;
	}
	MetricResult calc()
	{
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}
		int actualLatency = -1;
		int actualFrames = -1;
		int indexOfChange = -1;
		int IDOfChange = -1;
		int indexOfSet = -1;
		int IDOfSet = -1;
		double tsOfChangeFrame = -1;
		Logger::getLogger().log("Calculating metric: " + _metricName + " on " + _profile.GetText(), "Metric");

		double fps;
		if (_autoExposureOff)
		{
			fps = getFPSByExposure(_prev_exposure);
		}
		else
		{
			fps = _profile.fps;
		}
		for (int i = 0; i < _frames.size(); i++)
		{
			if (_frames[i].systemTimestamp >= _changeTime)
			{
				IDOfSet = _frames[i - 1].ID;
				indexOfSet=  i;
				tsOfChangeFrame = _frames[i - 1].systemTimestamp;
				break;
			}
		}
		for (int i = indexOfSet; i < _frames.size(); i++)
		{
			if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
			{
				indexOfChange = i;
				//actualFrames = round(double((_frames[i - 1].systemTimestamp - tsOfChangeFrame)) / expectedDelta) + 1;
				// actualLatency = _frames[i].ID - indexOfSet;
				break;
			}
		}
		fps = getActualFPS(_frames, fps, _useSystemTs,indexOfChange, false);
		
		double expectedDelta = 1000 / fps;
		// Logger::getLogger().log("prev_exposure:" + to_string(_prev_exposure) + " - expectedDelta:" + to_string(expectedDelta) + " - FPS:" + to_string(fps), "Metric");
		
		for (int i = indexOfSet; i < _frames.size(); i++)
		{
			if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
			{
				IDOfChange = _frames[i].ID;
				actualFrames = round(double((_frames[i - 1].systemTimestamp - tsOfChangeFrame)) / expectedDelta) + 1;
				// actualLatency = _frames[i].ID - indexOfSet;
				break;
			}
		}
		actualLatency = IDOfChange - IDOfSet;
		if (actualFrames >= 0 && actualFrames <= _tolerance)
			r.result = true;
		else
			r.result = false;

		r.remarks = "Control name: " + _metaDataName + "\nRequested value: " + to_string(_value) +
					"\nControl Set at TS: " + to_string(_changeTime) + "\nFrame compatible with the TS: " + to_string(IDOfSet) +
					"\nControl Actually changed at frame: " + to_string(IDOfChange) + "\nTolerance: " + to_string(_tolerance) +
					"\nControl Latency By Index: " + to_string(actualLatency) + "\nControl Latency by system timestamp: " + to_string(actualFrames) + "\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(actualFrames);
		return r;
	}
};

class MetaDataCorrectnessMetric : public Metric
{
	
private:
	bool _autoExposureOff = false;
	int _currExp;
	double _changeTime;
	double _value;
	string _metaDataName;
public:
	MetaDataCorrectnessMetric()
	{
		_metricName = "MetaData Correctness";
	}
	void SetFrames(vector<Frame> f)
	{
		_frames = f;
	}
	void setParams(int tolerance)
	{
		_tolerance = tolerance;
	}
	void setParams(int tolerance, int currExp, double changeTime, string metaDataName, double value)
	{
		_tolerance = tolerance;
		_autoExposureOff = true;
		_currExp = currExp;
		_changeTime = changeTime;
		_value = value;
		_metaDataName = metaDataName;
	}
	MetricResult calc()
	{
		if (_profile.fps == 0)
			throw std::runtime_error("Missing profile in Metric");
		MetricResult r;
		if (_frames.size() == 0)
		{
			//throw std::runtime_error("Frames array is empty");
			r.result = false;
			r.remarks = "Error: Frames not arrived";
			Logger::getLogger().log("Frames Not arrived", "Metric", LOG_ERROR);
			r.value = "0";
			return r;
		}
		int indexOfChange;
		double fps;
		if (_autoExposureOff)
		{
			indexOfChange = -1;
			for (int i = 0; i < _frames.size(); i++)
			{
				if (_frames[i].frameMD.getMetaDataByString(_metaDataName) == _value)
				{
					indexOfChange = i+1;

					break;
				}
			}
			fps = getFPSByExposure(_currExp);
		}
		else
		{
			fps = _profile.fps;
			indexOfChange = 5;
		}
		fps = getActualFPS(_frames, fps, _useSystemTs, indexOfChange);
		Logger::getLogger().log("Calculating metric: " + _metricName + " on " + _profile.GetText(), "Metric");
		double actualDelta = 0;
		string text = "";
		r.result = true;
		int numberOfSameTSEvents =0;
		int indexOfFirstSameTSEvent = -1;
		int maxSequentialSameTSEvents=0;
		int currSequentialSameTSEvent=0;
		int CountOfSameTSEvents=0;

		int numberOfSameIndexEvents =0;
		int indexOfFirstSameIndexEvent = -1;
		int maxSequentialSameIndexEvents=0;
		int currSequentialSameIndexEvent=0;
		int CountOfSameIndexEvents=0;

		for (int i=indexOfChange; i< _frames.size(); i++)
		{
			
			int idDelta;
			if (_useSystemTs)
			{
				idDelta = _frames[i].ID - _frames[i - 1].ID;
				actualDelta = (_frames[i].systemTimestamp - _frames[i - 1].systemTimestamp);
				text = "Used System Timestamp to calculate this metric\n";
			}
			else
			{
				idDelta = _frames[i].frameMD.getMetaDataByString("frameId") - _frames[i - 1].frameMD.getMetaDataByString("frameId");
				actualDelta = (_frames[i].frameMD.getMetaDataByString("Timestamp") - _frames[i - 1].frameMD.getMetaDataByString("Timestamp")) / 1000.0;
			}
			// check if delta is zero
			if (actualDelta==0)
			{
				if (currSequentialSameTSEvent==0)
				{
					CountOfSameTSEvents++;
				}
				r.result = false;
				numberOfSameTSEvents++;
				if (indexOfFirstSameTSEvent==-1)
					indexOfFirstSameTSEvent=_frames[i].ID;
				currSequentialSameTSEvent++;
				if (currSequentialSameTSEvent > maxSequentialSameTSEvents)
				{
					maxSequentialSameTSEvents = currSequentialSameTSEvent;
				}
			}
			else
			{
				currSequentialSameTSEvent=0;
			}
		// check if ID Delta is zero
			if (idDelta==0)
			{
				if (currSequentialSameIndexEvent==0)
				{
					CountOfSameIndexEvents++;
				}
				r.result = false;
				numberOfSameIndexEvents++;
				if (indexOfFirstSameIndexEvent==-1)
					indexOfFirstSameIndexEvent=_frames[i].ID;
				currSequentialSameIndexEvent++;
				if (currSequentialSameIndexEvent > maxSequentialSameIndexEvents)
				{
					maxSequentialSameIndexEvents = currSequentialSameIndexEvent;
				}
			}
			else
			{
				currSequentialSameIndexEvent=0;
			}


		}

		r.remarks = text + "Index of first Same TS event: " + to_string(indexOfFirstSameTSEvent) + "\nNumber of Same TS events: " + to_string(CountOfSameTSEvents) +
					"\nMax length of same TS event: " + to_string(maxSequentialSameTSEvents) + "\nNumber of Same TS frames: " + to_string(numberOfSameTSEvents) +
					"\nIndex of first Same Index event: " + to_string(indexOfFirstSameIndexEvent) + "\nNumber of Same Index events: " + to_string(CountOfSameIndexEvents) +
					"\nMax length of same Index event: " + to_string(maxSequentialSameIndexEvents) + "\nNumber of Same Index frames: " + to_string(numberOfSameIndexEvents) +
					"\nMetric result: " + ((r.result) ? "Pass" : "Fail");
		vector<string> results = r.getRemarksStrings();
		for (int i = 0; i < results.size(); i++)
		{
			Logger::getLogger().log(results[i], "Metric");
		}
		r.value = to_string(numberOfSameIndexEvents + numberOfSameTSEvents);
		return r;
	}
};

class TestBase : public testing::Test
{
public:
	string name;
	string suiteName = "";
	string hostname;
	string IPaddress;
	string DriverVersion;
	int iterations = 0;
	int failediter = 0;

	ofstream resultCsv;
	ofstream rawDataCsv;
	ofstream iterationCsv;
	ofstream pnpCsv;

	int rawDataFirstIter=0;
	bool isPNPtest = false;
	bool testStatus = true;
	int testDuration;
	bool _saveRawData=true;
	string rawDataPath;
	// vector<Frame> depthFrames, irFrames, colorFrames;
	// vector<Sample> pnpSamples;
	Profile depthProfile, irProfile, colorProfile;
	vector<Metric *> metrics;
	vector<ContentMetric *> contentMetrics;
	vector<PnpMetric *> pnpMetrics;
	vector<string> depthNonMandatoryMetrics, irNonMandatoryMetrics, colorNonMandatoryMetrics, gyroNonMandatoryMetrics, accelNonMandatoryMetrics, pnpNonMandatoryMetrics;
	string testBasePath;
	Camera cam;

	void OpenRawDataCSV()
	{
		rawDataCsv.open(rawDataPath, std::ios_base::app);
		if (rawDataCsv.fail())
		{
			Logger::getLogger().log("Cannot open raw data file: " + rawDataPath, LOG_ERROR);
			throw std::runtime_error("Cannot open file: " + rawDataPath);
		}
		rawDataCsv << "Iteration,StreamCombination,Stream Type,Image Format,Resolution,FPS,Gain,AutoExposure,Exposure,LaserPowerMode,LaserPower,Frame Index,MetaData Index,HW TimeStamp-Frame,HWTS-MetaData,System TimeStamp" << endl;

	}
	void SetUp() override
	{

		struct passwd *pw = getpwuid(getuid());

		char *homedir = pw->pw_dir;
		memoryBaseLine = 0;
		// testBasePath = FileUtils::join(FileUtils::getHomeDir()+"/Logs",TimeUtils::getDateandTime());
		//if (FileUtils::isDirExist("/media/administrator/DataUSB/storage"))
		//{
		//	testBasePath = FileUtils::join("/media/administrator/DataUSB/storage/Logs", sid);
		//}
		//else
		//{
			testBasePath = FileUtils::join(FileUtils::getHomeDir()+"/Logs", sid);
		//}
		name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		suiteName = ::testing::UnitTest::GetInstance()->current_test_case()->name();

		// Creating test folder
		string testPath = FileUtils::join(testBasePath, name);
		FileUtils::makePath(testPath);

		// Initializing the test logger
		string logFilePath = FileUtils::join(testPath, "test.log");
		Logger::getLogger().configure(logFilePath, LOG_INFO, true);

		Logger::getLogger().log("Starting test Setup()", "Setup()", LOG_INFO);

		if (_saveRawData)
		{
			// Creating raw data csv file
			Logger::getLogger().log("Creating raw data CSV file", "Setup()", LOG_INFO);
			rawDataPath = FileUtils::join(testPath, "raw_data.csv");
			// rawDataCsv.open(rawDataPath, std::ios_base::app);
			// if (rawDataCsv.fail())
			// {
			// 	Logger::getLogger().log("Cannot open raw data file: " + rawDataPath, LOG_ERROR);
			// 	throw std::runtime_error("Cannot open file: " + rawDataPath);
			// }
			// rawDataCsv << "Iteration,StreamCombination,Stream Type,Image Format,Resolution,FPS,Gain,AutoExposure,Exposure,LaserPowerMode,LaserPower,Frame Index,MetaData Index,HW TimeStamp-Frame,HWTS-MetaData,System TimeStamp" << endl;
			OpenRawDataCSV();
		}

		// Creating iteration Summary data csv file
		Logger::getLogger().log("Creating iteration Summary CSV file", "Setup()", LOG_INFO);
		string iterationSummaryPath = FileUtils::join(testPath, "iteration_summary.csv");
		iterationCsv.open(iterationSummaryPath, std::ios_base::app);
		if (iterationCsv.fail())
		{
			Logger::getLogger().log("Cannot open iteration Summary file: " + iterationSummaryPath, LOG_ERROR);
			throw std::runtime_error("Cannot open file: " + iterationSummaryPath);
		}
		iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,BandWidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;

		if (isPNPtest)
		{
			Logger::getLogger().log("Creating PNP data CSV file", "Setup()", LOG_INFO);
			string pnpDataPath = FileUtils::join(testPath, "pnp_data.csv");
			pnpCsv.open(pnpDataPath, std::ios_base::app);
			if (pnpCsv.fail())
			{
				Logger::getLogger().log("Cannot open pnp data file: " + pnpDataPath, LOG_ERROR);
				throw std::runtime_error("Cannot open pnp file: " + pnpDataPath);
			}
			pnpCsv << "Iteration,Metric name,Average,Max,Min,Tolerance" << endl;
		}
		// Creating iterations results csv file
		Logger::getLogger().log("Creating iteartions results CSV file", "Setup()", LOG_INFO);
		string resultPath = FileUtils::join(testPath, "result.csv");
		resultCsv.open(resultPath, std::ios_base::app);
		if (resultCsv.fail())
		{
			Logger::getLogger().log("Cannot open result file: " + resultPath, LOG_ERROR);
			throw std::runtime_error("Cannot open file: " + resultPath);
		}
		resultCsv << "Iteration,Test name, Test Suite, Camera Serial,Stream Combination,Stream Duration,Tested Stream,Metric name,Metric status,Remarks,Iteration status" << endl;

		// update Host name
		char temp_hostname[HOST_NAME_MAX];
		gethostname(temp_hostname, HOST_NAME_MAX);
		hostname = temp_hostname;
		// hostname = "testPC";

		//update IP address (still not impelemnted
		IPaddress = gethostIP();

		//get DriverVersion
		DriverVersion = getDriverVersion();
		Logger::getLogger().log("Host Name: " + hostname, "Setup()");
		Logger::getLogger().log("Host IP: " + IPaddress, "Setup()");
		Logger::getLogger().log("Driver version: " + DriverVersion, "Setup()");
		Logger::getLogger().log("Initializing camera", "Setup()");
		cam.Init();
		Logger::getLogger().log("Camera Serial:" + cam.GetSerialNumber(), "Setup()");
		Logger::getLogger().log("Camera FW version:" + cam.GetFwVersion(), "Setup()");

		bool rstDefaults = ResetDefaults();
		if (!rstDefaults)
			Logger::getLogger().log("Failed to Reset default controls", "Setup()", LOG_WARNING);
	}
	void TearDown() override
	{
		Logger::getLogger().log("Closing iteartions results CSV file", "TearDown()", LOG_INFO);
		resultCsv.close();
		
		if (_saveRawData)
		{
			Logger::getLogger().log("Closing raw data CSV file", "TearDown()", LOG_INFO);
			rawDataCsv.close();
			

			if (rawDataFirstIter>0)
			{
				//rawDataPath = FileUtils::join(testPath, "raw_data.csv");
				string testPath =FileUtils::join(testBasePath, name);
				string newname=FileUtils::join(testPath, "raw_data_"+to_string(rawDataFirstIter)+"_End.csv");
				if (rename(rawDataPath.c_str(),newname.c_str())!=0)
				{
					Logger::getLogger().log("Failed to rename RawData file", "Test", LOG_ERROR);
				}
				else
				{
					Logger::getLogger().log("Renamed Raw Data CSV to: "+ newname , "Test");
				}
			}
		}
		Logger::getLogger().log("Closing iteartions results CSV file", "TearDown()", LOG_INFO);
		iterationCsv.close();

		string TestsResultsPath = FileUtils::join(testBasePath, "tests_results.csv");
		ofstream TestsResults;
		TestsResults.open(TestsResultsPath, std::ios_base::app);
		if (!is_tests_res_created)
		{
			is_tests_res_created = true;
			TestsResults << "SID"
					     << ","
						 << "Camera Serial"
						 << ","
						 << "Test name"
						 << ","
						 << "Test Suite"
						 << ","
						 << "Test status"
						 << ","
						 << "Total iterations#"
						 << ","
						 << "Failed Iterations#"
						 << ","
						 << "Pass rate" << endl;
		}
		if (testStatus)
			TestsResults << sid << ","<< cam.GetSerialNumber() << "," << name << "," << suiteName << ","
						 << "Pass," << iterations << "," << failediter << "," << to_string(100 * double(iterations - failediter) / iterations) << endl;
		else
			TestsResults << sid << "," << cam.GetSerialNumber() << "," << name << "," << suiteName << ","
						 << "Fail," << iterations << "," << failediter << "," << to_string(100 * double(iterations - failediter) / iterations) << endl;
		TestsResults.close();
		Logger::getLogger().log("Closing Log file", "TearDown()", LOG_INFO);
		Logger::getLogger().close();
		CopyFolderToUSB();
	}


	void CopyFolderToUSB()
	{
		if (FileUtils::isDirExist("/media/administrator/DataUSB/storage"))
		{
			
			if (!FileUtils::isDirExist("/media/administrator/DataUSB/storage/Logs"))
			{
				File_Utils::makePath("/media/administrator/DataUSB/storage/Logs");
			}
			ofstream copyToUSBLog;
			copyToUSBLog.open("/media/administrator/DataUSB/storage/Logs/USBLog.txt", std::ios_base::app);
			copyToUSBLog << "USB Device found, Copying Folder: " << testBasePath << endl;
			string command = "cp -r '" + testBasePath + "/.' '/media/administrator/DataUSB/storage/Logs/"+sid+"/'";
			copyToUSBLog << command << endl;
			string res = exec(command.c_str());
			copyToUSBLog << res<<endl;
			if (res=="")
			{
				copyToUSBLog << "copy process finished succesfully, removing original folder" << endl;
				command = "rm -r '" + testBasePath + "/" +name +"'";
				copyToUSBLog << command << endl;
				copyToUSBLog<< exec(command.c_str()) << endl;;
			}
			else
			{
				copyToUSBLog << "copy process Failed, Leaving original folder" << endl;
			}
			copyToUSBLog.close();

		}
		else
		{
			cout << "USB Not Device found, Logs remain at: " << testBasePath << endl;
		}
	}
	bool ResetDefaults()
	{
		bool result = true;
		bool res;
		Logger::getLogger().log("Reseting default controls", "Setup()", LOG_INFO);
		Sensor* depthSensor = cam.GetDepthSensor();
		Sensor* colorSensor = cam.GetColorSensor();

		//Logger::getLogger().log("Disabling AutoExposure priority for Color Sensor", "Setup()");
		//res = colorSensor.SetControl(V4L2_CID_EXPOSURE_AUTO_PRIORITY, 0);
		//Logger::getLogger().log("Disabling AutoExposure priority for Color Sensor: " + (string)(res ? "Passed" : "Failed"), "Setup()");
		//result = result && res;

		Logger::getLogger().log("Enabling AutoExposure for Color Sensor", "Setup()");
		res = colorSensor->SetControl(V4L2_CID_EXPOSURE_AUTO, 3);
		Logger::getLogger().log("Enabling AutoExposure for Color Sensor: " + (string)(res ? "Passed" : "Failed"), "Setup()");
		result = result && res;

		Logger::getLogger().log("Enabling AutoExposure for Depth Sensor", "Setup()");
		res = depthSensor->SetControl(V4L2_CID_EXPOSURE_AUTO, 3);
		Logger::getLogger().log("Enabling AutoExposure for Depth Sensor: " + (string)(res ? "Passed" : "Failed"), "Setup()");
		result = result && res;
		Logger::getLogger().log("Enabling Laser power mode for Depth Sensor", "Setup()");
		res = depthSensor->SetControl(DS5_CAMERA_CID_LASER_POWER, 1);
		Logger::getLogger().log("Enabling Laser power mode for Depth Sensor: " + (string)(res ? "Passed" : "Failed"), "Setup()");
		result = result && res;
		Logger::getLogger().log("Setting Laser Power to 150 for Depth Sensor", "Setup()");
		res = depthSensor->SetControl(DS5_CAMERA_CID_MANUAL_LASER_POWER, 150);
		Logger::getLogger().log("Setting Laser Power to 150 for Depth Sensor: " + (string)(res ? "Passed" : "Failed"), "Setup()");
		result = result && res;
		std::this_thread::sleep_for(std::chrono::seconds(2));
		return result;
	}

	bool AppendPNPDataCVS(string pnpDataLine)
	{
		try
		{
			pnpCsv << pnpDataLine << endl;
			return true;
		}
		catch (const std::exception &e)
		{
			Logger::getLogger().log("Failed to write to PNP Data file");
			return false;
		}
	}

	bool AppendRAwDataCVS(string rawDataLine)
	{
		try
		{
			rawDataCsv << rawDataLine << endl;
			return true;
		}
		catch (const std::exception &e)
		{
			Logger::getLogger().log("Failed to write to RawData file");
			return false;
		}
	}

	bool AppendIterationSummaryCVS(string rawDataLine)
	{
		// data structure example:
		//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,Bandwidth,ProfileCombination,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;

		try
		{
			iterationCsv << rawDataLine << endl;
			return true;
		}
		catch (const std::exception &e)
		{
			Logger::getLogger().log("Failed to write to Iteration summary file");
			return false;
		}
	}

	bool AppendIterationResultCVS(string iterationResult)
	{
		try
		{
			resultCsv << iterationResult << endl;
			return true;
		}
		catch (const std::exception &e)
		{
			Logger::getLogger().log("Failed to write to Results file");
			return false;
		}
	}

	void IgnoreMetric(string metricName, StreamType type)
	{
		switch (type)
		{
		case StreamType::Depth_Stream:
			depthNonMandatoryMetrics.push_back(metricName);
			break;
		case StreamType::IR_Stream:
			irNonMandatoryMetrics.push_back(metricName);
			break;
		case StreamType::Color_Stream:
			colorNonMandatoryMetrics.push_back(metricName);
			break;
		}
	}
	void IgnoreMetricAllStreams(string metricName)
	{
		depthNonMandatoryMetrics.push_back(metricName);
		irNonMandatoryMetrics.push_back(metricName);
		colorNonMandatoryMetrics.push_back(metricName);
	}
	void IgnorePNPMetric(string metricName)
	{
		pnpNonMandatoryMetrics.push_back(metricName);
	}

	vector<Profile> GetControlProfiles(StreamType streamType)
	{
		ProfileGenerator pG;
		vector<Profile> combinations = pG.GetControlsProfiles(streamType);
		return combinations;
	}

	vector<vector<Profile>> GetProfiles(vector<StreamType> streamTypes)
	{
		ProfileGenerator pG;
		vector<vector<Profile>> combinations = pG.GetCombintaions(streamTypes);
		return combinations;
	}
	vector<vector<Profile>> GetMixedCombintaions(vector<StreamType> streamTypes)
	{
		ProfileGenerator pG;
		vector<vector<Profile>> combinations = pG.GetMixedCombintaions(streamTypes);
		return combinations;
	}

	vector<Profile> GetHighestCombination(vector<StreamType> streamTypes)
	{
		ProfileGenerator pG;
		vector<Profile> combination = pG.GetHighestCombination(streamTypes);
		return combination;
	}
	vector<Profile> GetHighestCombination(vector<StreamType> streamTypes,int fps)
	{
		ProfileGenerator pG;
		vector<Profile> combination = pG.GetHighestCombination(streamTypes, fps);
		return combination;
	}

	vector<Profile> GetProfiles_old(StreamType streamType)
	{
		//===================================
		vector<Profile> profiles;
		if (streamType == StreamType::Depth_Stream)
		{
			Resolution r = {0};
			Profile dP;
			r.width = 640;
			r.height = 480;
			dP.pixelFormat = V4L2_PIX_FMT_Z16;
			dP.resolution = r;
			dP.fps = 30;
			dP.streamType = StreamType::Depth_Stream;
			profiles.push_back(dP);
			// second profile
			r = {0};
			r.width = 640;
			r.height = 480;
			dP.pixelFormat = V4L2_PIX_FMT_Z16;
			dP.resolution = r;
			dP.fps = 15;
			dP.streamType = StreamType::Depth_Stream;
			profiles.push_back(dP);
			// third profile
			r.width = 1280;
			r.height = 720;
			dP.pixelFormat = V4L2_PIX_FMT_Z16;
			dP.resolution = r;
			dP.fps = 15;
			dP.streamType = StreamType::Depth_Stream;
			profiles.push_back(dP);
			return profiles;
		}
		else if (streamType == StreamType::IR_Stream)
		{
			Resolution r = {0};
			Profile dP;
			r.width = 640;
			r.height = 480;
			dP.pixelFormat = V4L2_PIX_FMT_Y8I;
			dP.resolution = r;
			dP.fps = 30;
			dP.streamType = StreamType::IR_Stream;
			profiles.push_back(dP);
			// second profile
			r = {0};
			r.width = 640;
			r.height = 480;
			dP.pixelFormat = V4L2_PIX_FMT_Y8I;
			dP.resolution = r;
			dP.fps = 15;
			dP.streamType = StreamType::IR_Stream;
			profiles.push_back(dP);
			// third profile
			r.width = 1280;
			r.height = 720;
			dP.pixelFormat = V4L2_PIX_FMT_Y8I;
			dP.resolution = r;
			dP.fps = 15;
			dP.streamType = StreamType::IR_Stream;
			profiles.push_back(dP);
			return profiles;
		}
		else if (streamType == StreamType::Color_Stream)
		{
			Resolution r = {0};
			Profile dP;
			r.width = 640;
			r.height = 480;
			dP.pixelFormat = V4L2_PIX_FMT_YUYV;
			dP.resolution = r;
			dP.fps = 30;
			dP.streamType = StreamType::Color_Stream;
			profiles.push_back(dP);
			// second profile
			r = {0};
			r.width = 640;
			r.height = 480;
			dP.pixelFormat = V4L2_PIX_FMT_YUYV;
			dP.resolution = r;
			dP.fps = 15;
			dP.streamType = StreamType::Color_Stream;
			profiles.push_back(dP);
			// third profile
			r.width = 1280;
			r.height = 720;
			dP.pixelFormat = V4L2_PIX_FMT_YUYV;
			dP.resolution = r;
			dP.fps = 15;
			dP.streamType = StreamType::Color_Stream;
			profiles.push_back(dP);
			return profiles;
		}
	}

	bool CalcMetrics(int iteration)
	{
		string iterationStatus = "Pass";
		string rawline;
		vector<string> iterationResults;
		iterationResults.clear();
		vector<string> failedMetrics;
		failedMetrics.clear();
		string streams = "";
		double bandWidth = 0;
		if (currDepthProfile.fps != 0)
		{
			streams += "Depth";
			bandWidth += currDepthProfile.GetSize() * currDepthProfile.fps;
		}
		if (currIRProfile.fps != 0)
		{
			if (streams == "")
				streams += "IR";
			else
				streams += "+IR";
			bandWidth += currIRProfile.GetSize() * currIRProfile.fps;
		}
		if (currColorProfile.fps != 0)
		{
			if (streams == "")
				streams += "Color";
			else
				streams += "+Color";
			bandWidth += currColorProfile.GetSize() * currColorProfile.fps;
		}
		if (currIMUProfile.fps != 0)
		{
			if (streams == "")
				streams += "Imu";
			else
				streams += "+Imu";
			bandWidth += currIMUProfile.GetSize() * currIMUProfile.fps;
		}
		// update the result csv
		for (int i = 0; i < pnpMetrics.size(); i++)
		{
			if (pnpMetrics[i]->_metricName == "CPU Consumption")
				pnpMetrics[i]->configure(cpuSamples);
			else if (pnpMetrics[i]->_metricName == "Memory Consumption")
				pnpMetrics[i]->configure(memSamples);
			else if (pnpMetrics[i]->_metricName == "Asic Temperature")
				pnpMetrics[i]->configure(asicSamples);
			else if (pnpMetrics[i]->_metricName == "Projector Temperature")
				pnpMetrics[i]->configure(projectorSamples);

			PNPMetricResult r = pnpMetrics[i]->calc();
			if (r.result == false)
			{
				if (!stringIsInVector(pnpMetrics[i]->_metricName, pnpNonMandatoryMetrics))
					iterationStatus = "Fail";
				else
				{
					Logger::getLogger().log("Metric: " + pnpMetrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
				}

				failedMetrics.push_back("Metric: " + pnpMetrics[i]->_metricName + " Failed");
			}
			string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",PNP," + pnpMetrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
			iterationResults.push_back(iRes);

			rawline = "";
			rawline += to_string(iteration) + "," + pnpMetrics[i]->_metricName + "," + to_string(r.average) + "," + to_string(r.max) + "," + to_string(r.min) + "," + to_string(r.tolerance);

			AppendPNPDataCVS(rawline);
			rawline = "";
			//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
			rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
					   "," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
					   "," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
					   ",PNP, " + pnpMetrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

			AppendIterationSummaryCVS(rawline);
		}
		for (int i = 0; i < contentMetrics.size(); i++)
		{
			if (currDepthProfile.fps != 0)
			{
				contentMetrics[i]->configure(currDepthProfile, fa.get_depth_corrupted_results(), fa.get_depth_freeze_results());
				MetricResult r = contentMetrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(contentMetrics[i]->_metricName, depthNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + contentMetrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + contentMetrics[i]->_metricName + " Failed on Depth stream");
				}
				string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Depth," + contentMetrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
					"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
					"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
					",Depth, " + contentMetrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);

			}
			if (currIRProfile.fps != 0)
			{
				contentMetrics[i]->configure(currIRProfile, fa.get_infrared_corrupted_results(), fa.get_infrared_freeze_results());
				MetricResult r = contentMetrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(contentMetrics[i]->_metricName, irNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + contentMetrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + contentMetrics[i]->_metricName + " Failed on IR stream");
				}
				string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",IR," + contentMetrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID, Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
					"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
					"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
					",IR, " + contentMetrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);
			}
			if (currColorProfile.fps != 0)
			{
				contentMetrics[i]->configure(currColorProfile, fa.get_color_corrupted_results(), fa.get_color_freeze_results());
				contentMetrics[i]->_useSystemTs = false;
				// metrics[i]->_useSystemTs = true;
				MetricResult r = contentMetrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(contentMetrics[i]->_metricName, colorNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + contentMetrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + contentMetrics[i]->_metricName + " Failed on Color stream");
				}
				string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Color," + contentMetrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
					"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
					"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
					",Color, " + contentMetrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);
			}
		}
		// calculate metric using HW timeStamp
		for (int i = 0; i < metrics.size(); i++)
		{
			if (currDepthProfile.fps != 0)
			{
				metrics[i]->configure(currDepthProfile, depthFramesList);
				metrics[i]->_useSystemTs = false;
				MetricResult r = metrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(metrics[i]->_metricName, depthNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + metrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + metrics[i]->_metricName + " Failed on Depth stream");
				}
				string iRes = to_string(iteration) + "," + name  + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Depth," + metrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\","+to_string(bandWidth)+"," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						   "," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						   "," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						   ",Depth, " + metrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);
			}

			if (currIRProfile.fps != 0)
			{
				metrics[i]->configure(currIRProfile, irFramesList);
				metrics[i]->_useSystemTs = true;
				MetricResult r = metrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(metrics[i]->_metricName, irNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + metrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + metrics[i]->_metricName + " Failed on IR stream");
				}
				string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",IR," + metrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID, Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\","+ to_string(bandWidth) + ","  + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						   "," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						   "," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						   ",IR, " + metrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);
			}
			if (currColorProfile.fps != 0)
			{
				metrics[i]->configure(currColorProfile, colorFramesList);
				metrics[i]->_useSystemTs = false;
				MetricResult r = metrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(metrics[i]->_metricName, colorNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + metrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + metrics[i]->_metricName + " Failed on Color stream");
				}
				string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Color," + metrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						   "," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						   "," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						   ",Color, " + metrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);
			}
			if (currIMUProfile.fps != 0 && metrics[i]->_metricName != "ID Correctness")
			{
				metrics[i]->configure(currIMUProfile, accelFrameList);
				metrics[i]->_useSystemTs = false;
				MetricResult r = metrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(metrics[i]->_metricName, accelNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + metrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + metrics[i]->_metricName + " Failed on Accel stream");
				}
				string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Accel," + metrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						   "," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						   "," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						   ",Accel, " + metrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);
			}
			if (currIMUProfile.fps != 0 && metrics[i]->_metricName != "ID Correctness")
			{
				metrics[i]->configure(currIMUProfile, gyroFrameList);
				metrics[i]->_useSystemTs = false;
				MetricResult r = metrics[i]->calc();
				if (r.result == false)
				{
					if (!stringIsInVector(metrics[i]->_metricName, gyroNonMandatoryMetrics))
					{
						iterationStatus = "Fail";
					}
					else
					{
						Logger::getLogger().log("Metric: " + metrics[i]->_metricName + " Failed, but ignored because its in the ignore list", "Test");
					}
					failedMetrics.push_back("Metric: " + metrics[i]->_metricName + " Failed on Gyro stream");
				}
				string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Gyro," + metrics[i]->_metricName + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
				iterationResults.push_back(iRes);

				rawline = "";
				//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
				rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						   "," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						   "," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						   ",Gyro, " + metrics[i]->_metricName + "," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

				AppendIterationSummaryCVS(rawline);
			}
		}
		if (calcSystemTSMetrics)
		{
		// calculate metric using System timeStamp
			for (int i = 0; i < metrics.size(); i++)
			{
				if (currDepthProfile.fps != 0)
				{
					metrics[i]->configure(currDepthProfile, depthFramesList);
					metrics[i]->_useSystemTs = true;
					MetricResult r = metrics[i]->calc();
					if (r.result == false)
					{
						//if (!stringIsInVector(metrics[i]->_metricName, depthNonMandatoryMetrics))
						//{
						//	iterationStatus = "Fail";
						//}
						//else
						//{
						//	Logger::getLogger().log("Metric: " + metrics[i]->_metricName + "(System TS) Failed, but ignored because its in the ignore list", "Test");
						//}
						failedMetrics.push_back("Metric: " + metrics[i]->_metricName + "(System TS) Failed on Depth stream");
					}
					string iRes = to_string(iteration) + "," + name + "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Depth," + metrics[i]->_metricName + "(System TS)," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
					iterationResults.push_back(iRes);

					rawline = "";
					//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
					rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						",Depth, " + metrics[i]->_metricName + "(System TS)," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

					AppendIterationSummaryCVS(rawline);
				}

				if (currIRProfile.fps != 0)
				{
					metrics[i]->configure(currIRProfile, irFramesList);
					metrics[i]->_useSystemTs = true;
					MetricResult r = metrics[i]->calc();
					if (r.result == false)
					{
						//if (!stringIsInVector(metrics[i]->_metricName, irNonMandatoryMetrics))
						//{
						//	iterationStatus = "Fail";
						//}
						//else
						//{
						//	Logger::getLogger().log("Metric: " + metrics[i]->_metricName + "(System TS) Failed, but ignored because its in the ignore list", "Test");
						//}
						failedMetrics.push_back("Metric: " + metrics[i]->_metricName + "(System TS) Failed on IR stream");
					}
					string iRes = to_string(iteration) + "," + name +  "," + suiteName + "," + cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",IR," + metrics[i]->_metricName + "(System TS)," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
					iterationResults.push_back(iRes);

					rawline = "";
					//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID, Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
					rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						",IR, " + metrics[i]->_metricName + "(System TS)," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

					AppendIterationSummaryCVS(rawline);
				}
				if (currColorProfile.fps != 0)
				{
					metrics[i]->configure(currColorProfile, colorFramesList);
					metrics[i]->_useSystemTs = true;
					MetricResult r = metrics[i]->calc();
					if (r.result == false)
					{
						//if (!stringIsInVector(metrics[i]->_metricName, colorNonMandatoryMetrics))
						//{
						//	iterationStatus = "Fail";
						//}
						//else
						//{
						//	Logger::getLogger().log("Metric: " + metrics[i]->_metricName + "(System TS) Failed, but ignored because its in the ignore list", "Test");
						//}
						failedMetrics.push_back("Metric: " + metrics[i]->_metricName + "(System TS) Failed on Color stream");
					}
					string iRes = to_string(iteration)+","+name+","+suiteName+","+cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Color," + metrics[i]->_metricName + "(System TS)," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
					iterationResults.push_back(iRes);

					rawline = "";
					//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
					rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						",Color, " + metrics[i]->_metricName + "(System TS)," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

					AppendIterationSummaryCVS(rawline);
				}
				if (currIMUProfile.fps != 0 && metrics[i]->_metricName != "ID Correctness")
				{
					metrics[i]->configure(currIMUProfile, accelFrameList);
					metrics[i]->_useSystemTs = true;
					MetricResult r = metrics[i]->calc();
					if (r.result == false)
					{
						//if (!stringIsInVector(metrics[i]->_metricName, accelNonMandatoryMetrics))
						//{
						//	iterationStatus = "Fail";
						//}
						//else
						//{
						//	Logger::getLogger().log("Metric: " + metrics[i]->_metricName + "(System TS) Failed, but ignored because its in the ignore list", "Test");
						//}
						failedMetrics.push_back("Metric: " + metrics[i]->_metricName + "(System TS) Failed on Accel stream");
					}
					string iRes = to_string(iteration)+","+name+","+suiteName+","+cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Accel," + metrics[i]->_metricName + "(System TS)," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
					iterationResults.push_back(iRes);

					rawline = "";
					//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
					rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						",Accel, " + metrics[i]->_metricName + "(System TS)," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

					AppendIterationSummaryCVS(rawline);
				}
				if (currIMUProfile.fps != 0 && metrics[i]->_metricName != "ID Correctness")
				{
					metrics[i]->configure(currIMUProfile, gyroFrameList);
					metrics[i]->_useSystemTs = true;
					MetricResult r = metrics[i]->calc();
					if (r.result == false)
					{
						//if (!stringIsInVector(metrics[i]->_metricName, gyroNonMandatoryMetrics))
						//{
						//	iterationStatus = "Fail";
						//}
						//else
						//{
						//	Logger::getLogger().log("Metric: " + metrics[i]->_metricName + "(System TS) Failed, but ignored because its in the ignore list", "Test");
						//}
						failedMetrics.push_back("Metric: " + metrics[i]->_metricName + "(System TS) Failed on Accel stream");
					}
					string iRes = to_string(iteration)+","+name+","+suiteName+","+cam.GetSerialNumber() + ",\"" + streamComb + "\"," + to_string(testDuration) + ",Gyro," + metrics[i]->_metricName + "(System TS)," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";
					iterationResults.push_back(iRes);

					rawline = "";
					//iterationCsv << "Host name, IP, FW version,Serial number ,Driver version,SID,Test Name,Test Suite,Iteration,Duration,StreamCombination,ProfileCombination,Bandwidth,Depth Image Format,Depth Width,Depth Hight,Depth FPS,IR Image Format,IR Width,IR Hight,IR FPS,Color Image Format,Color Width,Color Hight,Color FPS,IMU FPS,Tested Stream, Metric name,Metric Value,Metric Status,Metric Remarks,Iteration Result" << endl;
					rawline += hostname + "," + IPaddress + "," + cam.GetFwVersion() + "," + cam.GetSerialNumber() + "," + DriverVersion + "," + sid + "," + name + "," + suiteName + "," + to_string(iteration) + "," + to_string(testDuration) + "," + streams + ",\"" + streamComb + "\"," + to_string(bandWidth) + "," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "," + to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) +
						"," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "," + to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) +
						"," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "," + to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(currIMUProfile.fps) +
						",Gyro, " + metrics[i]->_metricName + "(System TS)," + r.value + "," + ((r.result) ? "Pass" : "Fail") + ",\"" + r.remarks + "\",";

					AppendIterationSummaryCVS(rawline);
				}
			}
		}

		for (int j = 0; j < iterationResults.size(); j++)
		{
			AppendIterationResultCVS(iterationResults[j] + iterationStatus);
		}
		// Iteration,Test name, Test Suite, Camrera Serial,stream Combination, Stream Type,Image Format,Resolution,FPS,Frame Index,HW TimeStamp,System TimeStamp
		//  "Iteration,StreamCombination,Stream Type,Image Format,Resolution,FPS,Gain,AutoExposure,Exposure,LaserPowerMode,LaserPower,Frame Index,HW TimeStamp,System TimeStamp" << endl;
		if (_saveRawData)
		{
			if (currDepthProfile.fps != 0)
			{

				for (int i = 0; i < depthFramesList.size(); i++)
				{
					rawline = "";
					rawline += to_string(iteration) + ",\"" + streamComb + "\",Depth," + currDepthProfile.GetFormatText() + "," + to_string(currDepthProfile.resolution.width) + "x" +
							to_string(currDepthProfile.resolution.height) + "," + to_string(currDepthProfile.fps) + "," + to_string(depthFramesList[i].frameMD.getMetaDataByString("Gain")) + "," + to_string(depthFramesList[i].frameMD.getMetaDataByString("AutoExposureMode")) + "," + to_string(depthFramesList[i].frameMD.getMetaDataByString("exposureTime")) + "," + to_string(depthFramesList[i].frameMD.getMetaDataByString("LaserPowerMode")) + "," + to_string(depthFramesList[i].frameMD.getMetaDataByString("ManualLaserPower")) + "," + to_string(depthFramesList[i].ID) +  "," + to_string(depthFramesList[i].frameMD.getMetaDataByString("frameId")) +"," + to_string(depthFramesList[i].hwTimestamp) + "," + to_string(depthFramesList[i].frameMD.getMetaDataByString("Timestamp")) + "," + to_string(depthFramesList[i].systemTimestamp);
					AppendRAwDataCVS(rawline);
				}
			}
			if (currIRProfile.fps != 0)
			{

				for (int i = 0; i < irFramesList.size(); i++)
				{
					rawline = "";
					rawline += to_string(iteration) + ",\"" + streamComb + "\",IR," + currIRProfile.GetFormatText() + "," + to_string(currIRProfile.resolution.width) + "x" +
							to_string(currIRProfile.resolution.height) + "," + to_string(currIRProfile.fps) + "," + to_string(irFramesList[i].frameMD.getMetaDataByString("Gain")) + "," + to_string(irFramesList[i].frameMD.getMetaDataByString("AutoExposureMode")) + "," + to_string(irFramesList[i].frameMD.getMetaDataByString("exposureTime")) + "," + to_string(irFramesList[i].frameMD.getMetaDataByString("LaserPowerMode")) + "," + to_string(irFramesList[i].frameMD.getMetaDataByString("ManualLaserPower")) + "," + to_string(irFramesList[i].ID) + "," + to_string(irFramesList[i].frameMD.getMetaDataByString("frameId")) +"," + to_string(irFramesList[i].hwTimestamp) + "," + to_string(irFramesList[i].frameMD.getMetaDataByString("Timestamp")) + "," + to_string(irFramesList[i].systemTimestamp);

					AppendRAwDataCVS(rawline);
				}
			}
			if (currColorProfile.fps != 0)
			{

				for (int i = 0; i < colorFramesList.size(); i++)
				{
					rawline = "";
					rawline += to_string(iteration) + ",\"" + streamComb + "\",Color," + currColorProfile.GetFormatText() + "," + to_string(currColorProfile.resolution.width) + "x" +
							to_string(currColorProfile.resolution.height) + "," + to_string(currColorProfile.fps) + "," + to_string(colorFramesList[i].frameMD.getMetaDataByString("Gain")) + "," + to_string(colorFramesList[i].frameMD.getMetaDataByString("AutoExposureMode")) + "," + to_string(colorFramesList[i].frameMD.getMetaDataByString("exposureTime")) + "," + to_string(colorFramesList[i].frameMD.getMetaDataByString("LaserPowerMode")) + "," + to_string(colorFramesList[i].frameMD.getMetaDataByString("ManualLaserPower")) + "," + to_string(colorFramesList[i].ID) + "," + to_string(colorFramesList[i].frameMD.getMetaDataByString("frameId")) +"," + to_string(colorFramesList[i].hwTimestamp) + "," + to_string(colorFramesList[i].frameMD.getMetaDataByString("Timestamp")) + "," + to_string(colorFramesList[i].systemTimestamp);

					AppendRAwDataCVS(rawline);
				}
			}
			if (currIMUProfile.fps != 0)
			{

				for (int i = 0; i < gyroFrameList.size(); i++)
				{
					rawline = "";
					rawline += to_string(iteration) + ",\"" + streamComb + "\",Gyro," + currIMUProfile.GetFormatText() + "," + to_string(currIMUProfile.resolution.width) + "x" +
							to_string(currIMUProfile.resolution.height) + "," + to_string(currIMUProfile.fps) + "," + to_string(gyroFrameList[i].frameMD.getMetaDataByString("Gain")) + "," + to_string(gyroFrameList[i].frameMD.getMetaDataByString("AutoExposureMode")) + "," + to_string(gyroFrameList[i].frameMD.getMetaDataByString("exposureTime")) + "," + to_string(gyroFrameList[i].frameMD.getMetaDataByString("LaserPowerMode")) + "," + to_string(gyroFrameList[i].frameMD.getMetaDataByString("ManualLaserPower")) + "," + to_string(gyroFrameList[i].ID) + "," + to_string(gyroFrameList[i].frameMD.getMetaDataByString("frameId")) +"," + to_string(gyroFrameList[i].hwTimestamp) + "," + to_string(gyroFrameList[i].frameMD.getMetaDataByString("Timestamp")) + "," + to_string(gyroFrameList[i].systemTimestamp);

					AppendRAwDataCVS(rawline);
				}

				for (int i = 0; i < accelFrameList.size(); i++)
				{
					rawline = "";
					rawline += to_string(iteration) + ",\"" + streamComb + "\",Accel," + currIMUProfile.GetFormatText() + "," + to_string(currIMUProfile.resolution.width) + "x" +
							to_string(currIMUProfile.resolution.height) + "," + to_string(currIMUProfile.fps) + "," + to_string(accelFrameList[i].frameMD.getMetaDataByString("Gain")) + "," + to_string(accelFrameList[i].frameMD.getMetaDataByString("AutoExposureMode")) + "," + to_string(accelFrameList[i].frameMD.getMetaDataByString("exposureTime")) + "," + to_string(accelFrameList[i].frameMD.getMetaDataByString("LaserPowerMode")) + "," + to_string(accelFrameList[i].frameMD.getMetaDataByString("ManualLaserPower")) + "," + to_string(accelFrameList[i].ID) + "," + to_string(accelFrameList[i].frameMD.getMetaDataByString("frameId")) +"," + to_string(accelFrameList[i].hwTimestamp) + "," + to_string(accelFrameList[i].frameMD.getMetaDataByString("Timestamp")) + "," + to_string(accelFrameList[i].systemTimestamp);

					AppendRAwDataCVS(rawline);
				}
			}
			// Check size of RawData File
			if (rawDataCsv.tellp()>RawDataMAxSize)
			{
				Logger::getLogger().log("RAW DATA FILE SIZE is:" + to_string(rawDataCsv.tellp()) , "Test");
				Logger::getLogger().log("RAW DATA FILE SIZE is Greater than Max size -" + to_string(RawDataMAxSize) , "Test");
				rawDataCsv.close();
				//rawDataPath = FileUtils::join(testPath, "raw_data.csv");
				string testPath =FileUtils::join(testBasePath, name);
				string newname=FileUtils::join(testPath, "raw_data_"+to_string(rawDataFirstIter)+"_"+to_string(iteration)+".csv");
				if (rename(rawDataPath.c_str(),newname.c_str())!=0)
				{
					Logger::getLogger().log("Failed to rename RawData file", "Test", LOG_ERROR);
				}
				else
				{
					Logger::getLogger().log("Renamed Raw Data CSV to: "+ newname , "Test");
				}
				rawDataFirstIter = iteration+1;
				OpenRawDataCSV();

			}
		}
		Logger::getLogger().log("Iteration #" + to_string(iteration) + " Summary", "Test");
		Logger::getLogger().log("Iteration #" + to_string(iteration) + ":[" + iterationStatus + "]", "Test");
		iterations += 1;

		if (iterationStatus == "Pass")
			return true;
		else
		{
			for (int i = 0; i < failedMetrics.size(); i++)
			{
				Logger::getLogger().log(failedMetrics[i], "Test");
			}
			failediter += 1;
			return false;
		}
	}

	void CalculateMemoryBaseLine()
	{
		// vector<int> tempMemUsed;
		SystemMonitor sysMon;
		int mem, sum = 0;
		Logger::getLogger().log("Calculating Memory Base Line", "Test");
		for (int i = 0; i < 10; i++)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			mem = sysMon.get_used_mem();
			sum += mem;
			Logger::getLogger().log("measurement: " + to_string(i) + " out of 10 is:" + to_string(mem), "Test");
		}
		memoryBaseLine = sum / 10.0;
		Logger::getLogger().log("Memory Base Line was set to " + to_string(memoryBaseLine), "Test");
	}
};
