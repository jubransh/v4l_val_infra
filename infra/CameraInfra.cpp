using namespace std;
#include <string>
#include <iostream>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <sys/ioctl.h>
#include <vector>
#include <fcntl.h>
#include <chrono>
#include "Logger.cpp"
#include <unordered_map>

using namespace std::chrono;

#define UPDC32(octet, crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

#define NUMBER_OF_BUFFERS 8
#define DS5_STREAM_CONFIG_0 0x4000
#define DS5_CAMERA_CID_BASE (V4L2_CTRL_CLASS_CAMERA | DS5_STREAM_CONFIG_0)
#define DS5_CAMERA_CID_LASER_POWER (DS5_CAMERA_CID_BASE + 1)
#define DS5_CAMERA_CID_MANUAL_LASER_POWER (DS5_CAMERA_CID_BASE + 2)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET (DS5_CAMERA_CID_BASE + 3)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET (DS5_CAMERA_CID_BASE + 4)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET (DS5_CAMERA_CID_BASE + 5)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET (DS5_CAMERA_CID_BASE + 6)
#define DS5_CAMERA_CID_FW_VERSION (DS5_CAMERA_CID_BASE + 7)
#define DS5_CAMERA_CID_GVD (DS5_CAMERA_CID_BASE + 8)
#define DS5_CAMERA_CID_AE_ROI_GET (DS5_CAMERA_CID_BASE + 9)
#define DS5_CAMERA_CID_AE_ROI_SET (DS5_CAMERA_CID_BASE + 10)
#define DS5_CAMERA_CID_AE_SETPOINT_GET (DS5_CAMERA_CID_BASE + 11)
#define DS5_CAMERA_CID_AE_SETPOINT_SET (DS5_CAMERA_CID_BASE + 12)
#define DS5_CAMERA_CID_ERB (DS5_CAMERA_CID_BASE + 13)
#define DS5_CAMERA_CID_EWB (DS5_CAMERA_CID_BASE + 14)
#define DS5_CAMERA_CID_HWMC (DS5_CAMERA_CID_BASE + 15)
#define TEGRA_CAMERA_CID_VI_PREFERRED_STRIDE 0x9a206e

static const uint32_t crc_32_tab[] __attribute__((section(".rodata"))) = {/* CRC polynomial 0xedb88320 */
                                                                          0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
                                                                          0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
                                                                          0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
                                                                          0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
                                                                          0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
                                                                          0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
                                                                          0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
                                                                          0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
                                                                          0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
                                                                          0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
                                                                          0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
                                                                          0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
                                                                          0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
                                                                          0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
                                                                          0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
                                                                          0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
                                                                          0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
                                                                          0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
                                                                          0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
                                                                          0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
                                                                          0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
                                                                          0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
                                                                          0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
                                                                          0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
                                                                          0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
                                                                          0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
                                                                          0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
                                                                          0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
                                                                          0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
                                                                          0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
                                                                          0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
                                                                          0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
                                                                          0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
                                                                          0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
                                                                          0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
                                                                          0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
                                                                          0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
                                                                          0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
                                                                          0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
                                                                          0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
                                                                          0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
                                                                          0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
                                                                          0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

class TimeUtils
{
private:
public:
    static long getCurrentTimestamp()
    {
        using std::chrono::system_clock;
        auto currentTime = std::chrono::system_clock::now();
        auto transformed = currentTime.time_since_epoch().count() / 1000000;
        return transformed;
    }
    static string getDateandTime()
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        return oss.str();
    }
};
class Helper
{
private:
    int res = -1;
    bool openDone;

public:
};

enum StreamType
{
    Depth_Stream,
    IR_Stream,
    Color_Stream,
    Imu_Stream
};
enum SensorType
{
    Depth,
    IR,
    Color,
    IMU
};

typedef struct
{
    uint8_t typeID;
    uint8_t skip1;
    uint64_t hwTimestamp;
    int16_t x;
    int16_t y;
    int16_t z;
    uint64_t hwTimestamp_2;
    uint64_t skip2;
} __attribute__((packed)) IMUFrameData;

// struct IMUFrameData
// {
//     uint8_t typeID;
//     uint8_t skip1;
//     uint64_t hwTimestamp;
//     uint16_t x;
//     uint16_t y;
//     uint16_t z;
//     uint64_t hwTimestamp_2;
//     uint64_t skip2;
// };

struct Resolution
{
    uint32_t width;
    uint32_t height;
};

class Profile
{
private:
    int bpp;

public:
    uint32_t pixelFormat;
    Resolution resolution;
    uint32_t fps;
    int streamType;
    string GetText()
    {
        string sT;
        switch (streamType)
        {
        case StreamType::Depth_Stream:
            sT = "Depth";
            break;
        case StreamType::IR_Stream:
            sT = "IR";
            break;
        case StreamType::Color_Stream:
            sT = "Color";
            break;
        case StreamType::Imu_Stream:
            sT = "IMU";
            break;
        }
        string result = "";
        result += sT + ":";
        result += to_string(resolution.width) + "*" + to_string(resolution.height) + "@" + to_string(fps) + ":format:" + GetFormatText();
        return result;
    }

    string GetFormatText()
    {
        switch (pixelFormat)
        {
        case V4L2_PIX_FMT_GREY:
            return "Grey";
            break;
        case V4L2_PIX_FMT_Y8:
            return "Y8";
            break;
        case V4L2_PIX_FMT_Y8I:
            return "Y8i";
            break;
        case V4L2_PIX_FMT_Y12I:
            return "Y12I";
            break;
        case V4L2_PIX_FMT_Z16:
            return "Z16";
            break;
        case V4L2_PIX_FMT_YUYV:
            return "YUYV";
            break;
        case 0:
            if (streamType == StreamType::Imu_Stream)
                return "XYZ";
            else
                return "";
            break;
        default:
            return "";
            break;
        }
    }
    double GetSize()
    {
        if (streamType == StreamType::Imu_Stream)
        {
            return 32;
        }
        int bpp;
        switch (pixelFormat)
        {
        case V4L2_PIX_FMT_Y8:
            bpp = 1;
            break;
        case V4L2_PIX_FMT_GREY:
            bpp = 1;
            break;
        case V4L2_PIX_FMT_Y12I:
            bpp = 4;
            break;
        default:
            bpp = 2;
            break;
        }
        int bytes_per_line = 0;
        int actualWidth = resolution.width;
        if (resolution.width != 640 && resolution.width != 1280)
        {
            while (true)
            {
                if (actualWidth % 64 == 0)
                    break;
                else
                    actualWidth += 1;
            }
            bytes_per_line = actualWidth * bpp;
            return resolution.height * bytes_per_line;
        }
        return resolution.width * resolution.height * bpp;
    }

    int GetBpp()
    {
        switch (pixelFormat)
        {
        case V4L2_PIX_FMT_Y8:
            bpp = 1;
            break;
        case V4L2_PIX_FMT_GREY:
            bpp = 1;
            break;
        case V4L2_PIX_FMT_Y12I:
            bpp = 4;
            break;
        default:
            bpp = 2;
            break;
        }
        return bpp;
    }
};

struct HWMC
{
    uint16_t header;
    uint16_t magic_word;
    uint32_t opcode;
    uint32_t params[4];
};

struct HWMonitorCommand
{
    uint32_t opCode;
    uint32_t parameter1;
    uint32_t parameter2;
    uint32_t parameter3;
    uint32_t parameter4;
    uint32_t dataSize;
    __u8 *data;
};

struct CommandResult
{
    vector<__u8> Data;
    bool Result;
};

struct CommonMetadata
{
    int frameId = 0;
    uint32_t CRC = 0;
    double Size = 0;
    double Timestamp = 0;
    int Type = 0;
    bool DataCorrectness = false;
    double exposureTime = 0;
    double Gain = 0;
    double AutoExposure = 0;
    double AutoExposureMode = 0;
    double manualExposure = 0;
    double width = 0;
    double height = 0;
    bool CrcCorrectness = true;
};
struct ImuMetaData
{
    int imuType = 0;
    float x, y, z;
};

struct ColorMetadata
{
    double BackLighCompensation = 0;
    double Brightness = 0;
    double Contrast = 0;
    double Gamma = 0;
    double Hue = 0;
    double Saturation = 0;
    double Sharpness = 0;
    double WhiteBalance = 0;
    double LowLightCompensation = 0;
    double PowerLineFrequency = 0;
    double AutoWhiteBalanceTemp = 0;
};

struct DepthMetadata
{
    double preset = 0;
    double LaserPowerMode = 0;
    double ManualLaserPower = 0;
};

class Metadata
{
public:
    CommonMetadata commonMetadata;
    ColorMetadata colorMetadata;
    DepthMetadata depthMetadata;
    ImuMetaData imuMetadata;
    void print_MetaData()
    {
        string text = "commonMetadata values:";
        text += "frameID=" + to_string(commonMetadata.frameId);
        text += ", CRC=" + to_string(commonMetadata.CRC);
        text += ", Size=" + to_string(commonMetadata.Size);
        text += ", Timestamp=" + to_string(commonMetadata.Timestamp);
        text += ", Type=" + to_string(commonMetadata.Type);
        text += ", DataCorrectness=" + to_string(commonMetadata.DataCorrectness);
        text += ", exposureTime=" + to_string(commonMetadata.exposureTime);
        text += ", Gain=" + to_string(commonMetadata.Gain);
        text += ", AutoExposure=" + to_string(commonMetadata.AutoExposure);
        text += ", AutoExposureMode=" + to_string(commonMetadata.AutoExposureMode);
        text += ", manualExposure=" + to_string(commonMetadata.manualExposure);
        text += ", width=" + to_string(commonMetadata.width);
        text += ", height=" + to_string(commonMetadata.height);
        Logger::getLogger().log(text, LOG_INFO);

        text = "DepthMetadata values:";
        text += ", preset=" + to_string(depthMetadata.preset);
        text += ", LaserPowerMode=" + to_string(depthMetadata.LaserPowerMode);
        text += ", ManualLaserPower=" + to_string(depthMetadata.ManualLaserPower);
        Logger::getLogger().log(text, LOG_INFO);

        text = "ColorMetadata values:";
        text += ", BackLighCompensation=" + to_string(colorMetadata.BackLighCompensation);
        text += ", Brightness=" + to_string(colorMetadata.Brightness);
        text += ", Contrast=" + to_string(colorMetadata.Contrast);
        text += ", Gamma=" + to_string(colorMetadata.Gamma);
        text += ", Hue=" + to_string(colorMetadata.Hue);
        text += ", Saturation=" + to_string(colorMetadata.Saturation);
        text += ", Sharpness=" + to_string(colorMetadata.Sharpness);
        text += ", WhiteBalance=" + to_string(colorMetadata.WhiteBalance);
        text += ", LowLightCompensation=" + to_string(colorMetadata.LowLightCompensation);
        text += ", PowerLineFrequency=" + to_string(colorMetadata.PowerLineFrequency);
        text += ", AutoWhiteBalanceTemp=" + to_string(colorMetadata.AutoWhiteBalanceTemp);
        Logger::getLogger().log(text, LOG_INFO);
        text = "IMUMetadata values:";
        text += ", imuType=" + to_string(imuMetadata.imuType);
        text += ", x=" + to_string(imuMetadata.x);
        text += ", y=" + to_string(imuMetadata.y);
        text += ", z=" + to_string(imuMetadata.z);

        Logger::getLogger().log(text, LOG_INFO);
    }

    double getMetaDataByString(string name)
    {
        if (name == "frameId")
            return commonMetadata.frameId;
        else if (name == "imuType")
            return imuMetadata.imuType;
        else if (name == "x")
            return imuMetadata.x;
        else if (name == "y")
            return imuMetadata.y;
        else if (name == "z")
            return imuMetadata.z;
        else if (name == "CRC")
            return commonMetadata.CRC;
        else if (name == "exposureTime")
            return commonMetadata.exposureTime;
        else if (name == "Gain")
            return commonMetadata.Gain;
        else if (name == "width")
            return commonMetadata.width;
        else if (name == "height")
            return commonMetadata.height;
        else if (name == "preset")
            return depthMetadata.preset;
        else if (name == "LaserPowerMode")
            return depthMetadata.LaserPowerMode;
        else if (name == "ManualLaserPower")
            return depthMetadata.ManualLaserPower;
        else if (name == "AutoExposure")
            return commonMetadata.AutoExposure;
        else if (name == "AutoExposureMode")
            return commonMetadata.AutoExposureMode;
        else if (name == "manualExposure")
            return commonMetadata.manualExposure;
        else if (name == "Size")
            return commonMetadata.Size;
        else if (name == "Timestamp")
            return commonMetadata.Timestamp;
        else if (name == "Type")
            return commonMetadata.Type;
        else if (name == "DataCorrectness")
            return commonMetadata.DataCorrectness;
        else if (name == "BackLighCompensation")
            return colorMetadata.BackLighCompensation;
        else if (name == "Brightness")
            return colorMetadata.Brightness;
        else if (name == "Contrast")
            return colorMetadata.Contrast;
        else if (name == "Gamma")
            return colorMetadata.Gamma;
        else if (name == "Hue")
            return colorMetadata.Hue;
        else if (name == "Saturation")
            return colorMetadata.Saturation;
        else if (name == "Sharpness")
            return colorMetadata.Sharpness;
        else if (name == "WhiteBalance")
            return colorMetadata.WhiteBalance;

        else if (name == "LowLightCompensation")
            return colorMetadata.LowLightCompensation;
        else if (name == "PowerLineFrequency")
            return colorMetadata.PowerLineFrequency;
        else if (name == "AutoWhiteBalanceTemp")
            return colorMetadata.AutoWhiteBalanceTemp;

        Logger::getLogger().log("Failed to get MetaData : " + name, LOG_ERROR);
        throw std::runtime_error("Failed to get MetaData : " + name);
    }
};

struct Frame
{
    uint8_t *Buff;
    Metadata frameMD;
    long systemTimestamp;
    long hwTimestamp;
    int ID;
    int streamType;
    __u32 size;
    __u32 buffSize;
};

class Sensor
{
    // For debugging
    void PrintBytes(uint8_t buff[], int len)
    {

        for (int i = 0; i < len; i++)
        {
            if (i % 16 == 0)
                cout << endl;

            cout << hex << unsigned(buff[i]) << " ";
        }
    }

private:
    bool isClosed = true;
    std::shared_ptr<std::thread> _t;
    bool cameraBusy = false;
    int dataFileDescriptor = 0;
    int metaFileDescriptor = 0;

    bool metaFileOpened = false;
    bool dataFileOpened = false;
    bool openMetaD = true;
    int insideIOCTL = 0;

    string name;
    SensorType type;
    bool stopRequested = false;
    bool isStarted = false;
    Profile lastProfile;
    std::array<void *, NUMBER_OF_BUFFERS> framesBuffer{};
    std::array<void *, NUMBER_OF_BUFFERS> metaDataBuffers{};

    int Open_timeout(const char *__path, int __oflag, int timeput_ms)
    {
        typedef unordered_map<string, pthread_t> ThreadMap;
        ThreadMap tm;
        string t_name = "open_thread";
        int result = -1;
        bool isDone = false;

        // thread open_thread(open, __path,__oflag);
        thread open_thread([&]()
                           {
                               result = open(__path, __oflag);
                               Logger::getLogger().log("Open Sensor Succeeded", "Sensor");
                               isDone = true; });

        tm[t_name] = open_thread.native_handle();
        open_thread.detach();

        auto delta = 0;
        auto t1 = TimeUtils::getCurrentTimestamp();
        while (!isDone)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            auto t2 = TimeUtils::getCurrentTimestamp();
            delta += t2 - t1;
            if (delta > timeput_ms)
            {
                // Kill the open thread
                Logger::getLogger().log("Open Sensor Stuck for more than " + to_string(timeput_ms) + "ms - Task killed", "Sensor");
                ThreadMap::const_iterator it = tm.find(t_name);
                if (it != tm.end())
                {
                    pthread_cancel(it->second);
                    tm.erase(t_name);
                }

                break;
            }
            t1 = t2;
        }

        return result;
    }

    int ioct_timeout(string buffType, int fd, unsigned long int request, v4l2_buffer *buff, int timeput_ms)
    {
        typedef unordered_map<string, pthread_t> ThreadMap;
        ThreadMap tm;
        string t_name = "ioctl_thread";
        int result = -1;
        bool isDone = false;
        insideIOCTL++;
        // thread open_thread(open, __path,__oflag);
        thread open_thread([&]()
                           {
                                result = ioctl(fd, request, buff);
                                isDone = true; });

        tm[t_name] = open_thread.native_handle();
        open_thread.detach();

        auto delta = 0;
        auto startTime = TimeUtils::getCurrentTimestamp();
        int i = 0;

        while (true)
        {

            // if (stopRequested)
            //{
            //     Logger::getLogger().log(buffType + "Stop was requested While waiting for IOCTL", "Sensor");
            //     break;
            // }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (++i % 1000 == 0)
                Logger::getLogger().log(buffType + "ioct_timeout still running", "Sensor");

            auto delta = TimeUtils::getCurrentTimestamp() - startTime;
            if (delta > timeput_ms)
            {
                Logger::getLogger().log(buffType + " Timeout exceeded - Delta = " + to_string(delta), "Sensor", LOG_ERROR);
                break;
            }

            if (isDone)
                break;
        }

        if (!isDone) // the ioctl still stuck
        {
            // Kill the open thread
            Logger::getLogger().log(buffType + " did not arrive within " + to_string(timeput_ms) + "ms - Task killed", "Sensor", LOG_ERROR);
            ThreadMap::const_iterator it = tm.find(t_name);
            if (it != tm.end())
            {
                pthread_cancel(it->second);
                tm.erase(t_name);
                Logger::getLogger().log(buffType + " Task killed", "Sensor", LOG_ERROR);
            }
        }
        insideIOCTL--;
        return result;
    }

    void requestBuffers(uint8_t fd, uint32_t type, uint32_t memory, uint32_t count)
    {
        struct v4l2_requestbuffers v4L2ReqBufferrs
        {
            0
        };
        v4L2ReqBufferrs.type = type;
        v4L2ReqBufferrs.memory = memory;
        v4L2ReqBufferrs.count = count;
        int ret = ioctl(fd, VIDIOC_REQBUFS, &v4L2ReqBufferrs);
        string t = "";
        if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
            t = "Frame ";
        else if (type == V4L2_BUF_TYPE_META_CAPTURE)
            t = "MetaData ";

        Logger::getLogger().log(GetName() + " Requesting " + t + "buffers " + string((0 == ret) ? "Succeeded" : "Failed"), "Sensor", LOG_INFO);
        Logger::getLogger().log(GetName() + " Requested buffers number: " + to_string(v4L2ReqBufferrs.count), "Sensor", LOG_INFO);

        ASSERT_TRUE(0 == ret);
        ASSERT_TRUE(0 < v4L2ReqBufferrs.count);
    }

    void *queryMapQueueBuf(uint8_t fd, uint32_t type, uint32_t memory, uint8_t index, uint32_t size)
    {
        struct v4l2_buffer v4l2Buffer
        {
            0
        };
        v4l2Buffer.type = type;
        v4l2Buffer.memory = memory;
        v4l2Buffer.index = index;
        int ret = ioctl(fd, VIDIOC_QUERYBUF, &v4l2Buffer);
        if (ret)
            return nullptr;

        void *buffer = (struct buffer *)mmap(nullptr,
                                             size,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED,
                                             fd,
                                             v4l2Buffer.m.offset);
        if (nullptr == buffer)
            return nullptr;

        // queue buffers
        ret = ioctl(fd, VIDIOC_QBUF, &v4l2Buffer);
        if (ret)
            nullptr;

        return buffer;
    }

    enum v4l2_buf_type vType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    enum v4l2_buf_type mdType = V4L2_BUF_TYPE_META_CAPTURE;

public:
    bool getIsClosed()
    {
        return isClosed;
    }
    bool getOpenMetaD()
    {
        return openMetaD;
    }
    SensorType getType()
    {
        return type;
    }
    bool copyFrameData = false;
    int GetFileDescriptor()
    {
        return dataFileDescriptor;
    }

    string GetName()
    {
        return name;
    }

    bool Init(SensorType sT, bool openMD)
    {
        openMetaD = openMD;
        string videoNode = "";
        type = sT;
        switch (sT)
        {
        case SensorType::Depth:
        {
            videoNode = {"/dev/video0"};
            Logger::getLogger().log("Openning /dev/video0", "Sensor");

            dataFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
            // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);

            if (openMD)
            {
                Logger::getLogger().log("Openning /dev/video1", "Sensor");
                videoNode = {"/dev/video1"};
                metaFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
                // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);
            }
            name = "Depth Sensor";
            dataFileOpened = dataFileDescriptor > 0;
            metaFileOpened = metaFileDescriptor > 0;
            isClosed = !dataFileOpened;

            Logger::getLogger().log("Init Sensor Depth Done", "Sensor");
            return dataFileOpened;
        }
        case SensorType::IR:
        {
            videoNode = {"/dev/video4"};
            Logger::getLogger().log("Openning /dev/video4", "Sensor");

            dataFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
            // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);

            // if (openMD)
            //{
            //     Logger::getLogger().log("Openning /dev/video5", "Sensor");
            //     videoNode = {"/dev/video5"};
            //     metaFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
            //     // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);
            // }
            name = "IR Sensor";
            dataFileOpened = dataFileDescriptor > 0;
            metaFileOpened = metaFileDescriptor > 0;
            isClosed = !dataFileOpened;

            Logger::getLogger().log("Init Sensor IR Done", "Sensor");
            return dataFileOpened;
        }

        case SensorType::Color:
        {
            Logger::getLogger().log("Openning /dev/video2", "Sensor");
            videoNode = {"/dev/video2"};
            dataFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
            // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);

            if (openMD)
            {
                Logger::getLogger().log("Openning /dev/video3", "Sensor");
                videoNode = {"/dev/video3"};
                metaFileDescriptor = open(videoNode.c_str(), O_RDWR);
            }
            name = "Color Sensor";
            dataFileOpened = dataFileDescriptor > 0;
            metaFileOpened = metaFileDescriptor > 0;
            isClosed = !dataFileOpened;

            Logger::getLogger().log("Init Sensor Color Done", "Sensor");
            return dataFileOpened;
        }
        case SensorType::IMU:
        {
            videoNode = {"/dev/video5"};
            Logger::getLogger().log("Openning /dev/video5", "Sensor");

            dataFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
            // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);

            // if (openMD)
            //{
            //     Logger::getLogger().log("Openning /dev/video5", "Sensor");
            //     videoNode = {"/dev/video5"};
            //     metaFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
            //     // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);
            // }
            name = "IMU Sensor";
            dataFileOpened = dataFileDescriptor > 0;
            metaFileOpened = metaFileDescriptor > 0;
            isClosed = !dataFileOpened;

            Logger::getLogger().log("Init Sensor IMU Done", "Sensor");
            return dataFileOpened;
        }
            return false;
        }
    }

    double GetControl(__u32 controlId)
    {
        bool wasClosed;
        if (isClosed)
            wasClosed = true;
        else
            wasClosed = false;
        if (wasClosed)
            Init(getType(), getOpenMetaD());
        struct v4l2_ext_control control
        {
            0
        };
        control.id = controlId;
        control.size = 0;

        struct v4l2_ext_controls ext
        {
            0
        };
        if (controlId == V4L2_CID_ANALOGUE_GAIN)
            ext.ctrl_class = V4L2_CTRL_CLASS_IMAGE_SOURCE;
        else
            ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &control;
        ext.count = 1;

        // get Control
        ext.controls = &control;
        ext.count = 1;
        int ret = ioctl(dataFileDescriptor, VIDIOC_G_EXT_CTRLS, &ext);
        if (wasClosed)
            Close();
        if (0 != ret)
        {
            // throw std::runtime_error("Failed to get control " + controlId);
            Logger::getLogger().log("Getting Control Failed - value Set to -1", "Test", LOG_ERROR);
            return -1;
        }

        else
            return control.value;
    }

    bool SetControl(__u32 controlId, __u32 value)
    {
        struct v4l2_ext_control control
        {
            0
        };
        control.id = controlId;
        control.size = 0;
        control.value = value;

        // set Control
        struct v4l2_ext_controls ext
        {
            0
        };
        bool wasClosed;
        if (isClosed)
            wasClosed = true;
        else
            wasClosed = false;
        if (wasClosed)
            Init(getType(), getOpenMetaD());
        if (controlId == V4L2_CID_ANALOGUE_GAIN)
            ext.ctrl_class = V4L2_CTRL_CLASS_IMAGE_SOURCE;
        else
            ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &control;
        ext.count = 1;
        int ret = ioctl(dataFileDescriptor, VIDIOC_S_EXT_CTRLS, &ext);
        if (wasClosed)
            Close();
        return (0 == ret);
    }

    vector<Resolution> GetSupportedResolutions()
    {
        __u32 format;
        if (type == SensorType::Depth)
            format = V4L2_PIX_FMT_Z16;
        else if (type == SensorType::IR)
            format = V4L2_PIX_FMT_Y8;
        else if (type == SensorType::Color)
            format = V4L2_PIX_FMT_YUYV;

        std::vector<Resolution> suppotedResolutions;
        v4l2_frmsizeenum frameSize{
            .index = 0,
            .pixel_format = format};

        while (0 == ioctl(dataFileDescriptor, VIDIOC_ENUM_FRAMESIZES, &frameSize))
        {
            // ASSERT_TRUE(V4L2_FRMSIZE_TYPE_DISCRETE == frameSize.type);
            Resolution res = {
                .width = frameSize.discrete.width,
                .height = frameSize.discrete.height};

            suppotedResolutions.push_back(res);
            frameSize.index++;
        }
        return suppotedResolutions;
    }

    vector<uint32_t> GetSupportedFormats()
    {
        vector<uint32_t> formats;
        struct v4l2_fmtdesc fmt = {0};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while (ioctl(dataFileDescriptor, VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            formats.push_back(fmt.pixelformat);
            fmt.index++;
        }
        return formats;
    }

    void Configure(Profile p)
    {
        int ret;
        if (isClosed)
            Init(type, openMetaD);
        if (GetName() == "IMU Sensor")
        {
            // Set the fps
            struct v4l2_streamparm setFps
            {
                0
            };
            setFps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            setFps.parm.capture.timeperframe.numerator = 1;
            setFps.parm.capture.timeperframe.denominator = p.fps;
            ret = ioctl(dataFileDescriptor, VIDIOC_S_PARM, &setFps);
            lastProfile = p;
            if (0 != ret)
            {
                Logger::getLogger().log("Failed to set Fps", "Sensor", LOG_ERROR);
                throw std::runtime_error("Failed to set Fps");
            }
            else
            {
                Logger::getLogger().log("Done configuring Sensor:" + GetName() + " with Profile: " + p.GetText(), "Sensor");
            }
            return;
        }
        // Set Stream profile
        struct v4l2_format sFormat = {0};
        sFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        // sFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_Z16;
        sFormat.fmt.pix.pixelformat = p.pixelFormat;
        sFormat.fmt.pix.width = p.resolution.width;
        sFormat.fmt.pix.height = p.resolution.height;
        ret = ioctl(dataFileDescriptor, VIDIOC_S_FMT, &sFormat);
        if (0 != ret)
        {
            Logger::getLogger().log("Failed to set Stream format", "Sensor", LOG_ERROR);
            throw std::runtime_error("Failed to set Stream format");
        }
        lastProfile = p;

        // Set the fps
        struct v4l2_streamparm setFps
        {
            0
        };
        setFps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        setFps.parm.capture.timeperframe.numerator = 1;
        setFps.parm.capture.timeperframe.denominator = p.fps;
        ret = ioctl(dataFileDescriptor, VIDIOC_S_PARM, &setFps);
        if (0 != ret)
        {
            Logger::getLogger().log("Failed to set Fps", "Sensor", LOG_ERROR);
            throw std::runtime_error("Failed to set Fps");
        }
        int bytes_per_line = 0;
        if (p.resolution.width != 640 && p.resolution.width != 1280)
        {
            int actualWidth = p.resolution.width;
            while (true)
            {
                if (actualWidth % 64 == 0)
                    break;
                else
                    actualWidth += 1;
            }
            bytes_per_line = actualWidth * p.GetBpp();
            // bytes_per_line=896
        }
        Logger::getLogger().log(GetName() + " Configuring Stride to : " + to_string(bytes_per_line), "Sensor");
        struct v4l2_control setStride;
        setStride.id = TEGRA_CAMERA_CID_VI_PREFERRED_STRIDE;
        setStride.value = bytes_per_line; // this should be 64 aligned and large enough for width * bpp, for example for Y8 848 set bytes_per_line to 896.
        ret = ioctl(dataFileDescriptor, VIDIOC_S_CTRL, &setStride);
        if (0 != ret)
        {
            Logger::getLogger().log(GetName() + " Failed to Configure Stride to : " + to_string(bytes_per_line), "Sensor", LOG_ERROR);
            throw std::runtime_error("Failed to set Fps");
        }
        else
            Logger::getLogger().log(GetName() + " Done Configuring Stride to : " + to_string(bytes_per_line), "Sensor");
        Logger::getLogger().log("Done configuring Sensor:" + GetName() + " with Profile: " + p.GetText(), "Sensor");
    }

    void Start(void (*FramesCallback)(Frame f))
    {
        Logger::getLogger().log("Starting Sensor: " + GetName(), "Sensor");
        insideIOCTL = 0;
        cameraBusy = true;
        isStarted = false;
        stopRequested = false;
        _t = std::make_shared<std::thread>([this, FramesCallback]()
                                           {
                                               // request buffers
                                               requestBuffers(dataFileDescriptor, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, NUMBER_OF_BUFFERS);

                                               if (metaFileOpened)
                                                   requestBuffers(metaFileDescriptor, V4L2_BUF_TYPE_META_CAPTURE, V4L2_MEMORY_MMAP, NUMBER_OF_BUFFERS);

                                               for (int i = 0; i < framesBuffer.size(); ++i)
                                               {
                                                   //framesBuffer[i] = queryMapQueueBuf(dataFileDescriptor,
                                                    //                                  V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                                     //                                 V4L2_MEMORY_MMAP,
                                                      //                                i,
                                                       //                               lastProfile.GetBpp() * lastProfile.resolution.width * lastProfile.resolution.height);
                                                   framesBuffer[i] = queryMapQueueBuf(dataFileDescriptor,
                                                       V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                                       V4L2_MEMORY_MMAP,
                                                       i,
                                                       lastProfile.GetSize());
                                                   if (metaFileOpened)
                                                       metaDataBuffers[i] = queryMapQueueBuf(metaFileDescriptor,
                                                                                             V4L2_BUF_TYPE_META_CAPTURE,
                                                                                             V4L2_MEMORY_MMAP,
                                                                                             i,
                                                                                             4096);
                                                   if (nullptr == framesBuffer[i])
                                                       throw std::runtime_error("Failed to allocate frames buffers\n");

                                                   if (metaFileOpened)
                                                       if (nullptr == metaDataBuffers[i])
                                                           Logger::getLogger().log("Failed to allocate MD buffers", "Sensor", LOG_WARNING);
                                               }

                                               // VIDIOC_STREAMON
                                                if (ioctl(dataFileDescriptor, VIDIOC_STREAMON, &vType) != 0)
                                                {
                                                    Logger::getLogger().log(GetName()+" Stream VIDIOC_STREAMON Failed - " + name, "Sensor", LOG_ERROR);
                                                    //    throw std::runtime_error("Failed to Start stream - " + name + "\n");
                                                }
                                                else
                                                {
                                                    isStarted = true;
                                                    Logger::getLogger().log(GetName() + " Stream VIDIOC_STREAMON Done", "Sensor");
                                                }
                                                
                                                if (metaFileOpened && isStarted)
                                               {
                                                   if (ioctl(metaFileDescriptor, VIDIOC_STREAMON, &mdType) != 0)
                                                   {
                                                       Logger::getLogger().log(GetName() + " Metadata VIDIOC_STREAMON Failed", "Sensor", LOG_ERROR);
                                                       //throw std::runtime_error("Failed to Start MD stream - " + name + "\n");
                                                   }
                                                   else
                                                   {
                                                       Logger::getLogger().log(GetName() + " Metadata VIDIOC_STREAMON Done", "Sensor");
                                                   }
                                               }

                                               long buffIndex = 0;
                                               try 
                                               {
                                                    while (!stopRequested)
                                                    {
                                                        if(!isStarted){
                                                            continue;
                                                        }

                                                        struct v4l2_buffer V4l2Buffer
                                                        {
                                                            0
                                                        };
                                                        V4l2Buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                                                        V4l2Buffer.memory = V4L2_MEMORY_MMAP;
                                                        //V4l2Buffer.index = buffIndex++ % 8;
                                                        struct v4l2_buffer mdV4l2Buffer
                                                        {
                                                            0
                                                        };

                                                        mdV4l2Buffer.type = V4L2_BUF_TYPE_META_CAPTURE;
                                                        mdV4l2Buffer.memory = V4L2_MEMORY_MMAP;

                                                        //V4l2Buffer.m.userptr = (unsigned long long) framesBuffer[V4l2Buffer.index];

                                                        //Read the frame buff freom the V4L //#TODO timeout mechanism
                                                        //    int res = ioctl(dataFileDescriptor, VIDIOC_DQBUF, &V4l2Buffer);
                                                        bool errorFlag = false;
                                                        bool frameReady = false;

                                                        if (stopRequested)
                                                        {
                                                            Logger::getLogger().log(name + " Stop is requested by user before Frame DQBUF - Exiting thread", "Sensor");
                                                            break;
                                                        }
                                                        fd_set fds;
                                                        struct timeval tv;
                                                        int r;

                                                        FD_ZERO(&fds);
                                                        FD_SET(dataFileDescriptor, &fds);

                                                        /* Timeout. */
                                                        tv.tv_sec = 5;
                                                        tv.tv_usec = 0;

                                                        r = select(dataFileDescriptor + 1, &fds, NULL, NULL, &tv);

                                                        if ( r == 0)
                                                        {
                                                            Logger::getLogger().log(name + " Select timed out " , "Sensor", LOG_ERROR);
                                                            continue;
                                                        }
                                                        if (r == -1)
                                                        {
                                                            Logger::getLogger().log(name + " Select Returned error ", "Sensor", LOG_ERROR);
                                                            continue;
                                                        }
                                                        //Frame ioctl
                                                        int res = ioct_timeout(name + " Frame", dataFileDescriptor, VIDIOC_DQBUF, &V4l2Buffer, 5000);
                                                        if (res != 0) 
                                                        {
                                                            Logger::getLogger().log(name + " Frame VIDIOC_DQBUF Failed on result: " + to_string(res), "Sensor", LOG_ERROR);
                                                             
                                                        }
                                                        else//if the return value = 0 
                                                        {
                                                            frameReady = true;
                                                        }
                                                        bool mdReady = false;
                                                        if (metaFileOpened)
                                                        {
                                                            FD_ZERO(&fds);
                                                            FD_SET(metaFileDescriptor, &fds);

                                                            /* Timeout. */
                                                            tv.tv_sec = 5;
                                                            tv.tv_usec = 0;

                                                            r = select(metaFileDescriptor + 1, &fds, NULL, NULL, &tv);

                                                            if (r == 0)
                                                            {
                                                                Logger::getLogger().log(name + " MetaData Select timed out ", "Sensor", LOG_ERROR);
                                                                continue;
                                                            }
                                                            if (r == -1)
                                                            {
                                                                Logger::getLogger().log(name + " MetaData Select Returned error ", "Sensor", LOG_ERROR);
                                                                continue;
                                                            }
                                                            res = ioct_timeout(name + " MD frame", metaFileDescriptor, VIDIOC_DQBUF, &mdV4l2Buffer, 5000);
                                                            if (res != 0)
                                                            {
                                                                errorFlag = true;
                                                                Logger::getLogger().log(name + " MD VIDIOC_DQBUF Failed on result: " + to_string(res), "Sensor", LOG_ERROR);
                                                            }
                                                            else
                                                                mdReady = true;
                                                        }
                                                        Frame frame;

                                                        //copy the frame content
                                                        if (copyFrameData)
                                                        {
                                                            int actualWidth = lastProfile.resolution.width;
                                                            if (lastProfile.resolution.width != 640 && lastProfile.resolution.width != 1280)
                                                            {
                                                                while (true)
                                                                {
                                                                    if (actualWidth % 64 == 0)
                                                                        break;
                                                                    else
                                                                        actualWidth += 1;
                                                                }
                                                            }
                                                            frame.buffSize = lastProfile.resolution.width * lastProfile.resolution.height * lastProfile.GetBpp();
                                                            frame.Buff = (uint8_t *)malloc(frame.buffSize);
                                                            // memcpy(frame.Buff, framesBuffer[V4l2Buffer.index], V4l2Buffer.bytesused);
                                                            for (int line = 0; line < lastProfile.resolution.height; line++)
                                                            {
                                                                memcpy(frame.Buff + line*lastProfile.resolution.width*lastProfile.GetBpp(), framesBuffer[V4l2Buffer.index] + line*actualWidth*lastProfile.GetBpp(), lastProfile.resolution.width*lastProfile.GetBpp());
                                                            }
                                                        }

                                                        frame.systemTimestamp = TimeUtils::getCurrentTimestamp();
                                                        frame.ID = V4l2Buffer.sequence;
                                                        frame.streamType = lastProfile.streamType;
                                                        frame.size = V4l2Buffer.bytesused;
                                                        frame.hwTimestamp = V4l2Buffer.timestamp.tv_usec;

                                                        Metadata md;
                                                        if (type == SensorType::IMU)
                                                        {
                                                                IMUFrameData* newPTR = static_cast<IMUFrameData*>(framesBuffer[V4l2Buffer.index]);
                                                                md.imuMetadata.imuType = newPTR->typeID;
                                                                md.imuMetadata.x = newPTR->x/100.0;
                                                                md.imuMetadata.y = newPTR->y/100.0;
                                                                md.imuMetadata.z = newPTR->z/100.0;
                                                                md.commonMetadata.frameId= V4l2Buffer.sequence;

                                                                md.commonMetadata.Timestamp = newPTR->hwTimestamp;

                                                               
                                                            //    if (newPTR->typeID == 2)
                                                            //     md.print_MetaData();
                                                         
                                                        
                                                        }
                                                        else if (metaFileOpened && mdReady)
                                                        {
                                                            md.commonMetadata.frameId = mdV4l2Buffer.sequence;
                                                            if (type == SensorType::Depth or type == SensorType::IR)
                                                            {
                                                                STMetaDataExtMipiDepthIR *ptr = static_cast<STMetaDataExtMipiDepthIR *>(metaDataBuffers[mdV4l2Buffer.index] + 16);

                                                                md.depthMetadata.preset = (uint16_t)ptr->preset;
                                                                md.depthMetadata.LaserPowerMode = (uint16_t)ptr->projectorMode;
                                                                md.depthMetadata.ManualLaserPower = ptr->laserPower;

                                                                md.commonMetadata.manualExposure = ptr->manualExposure;
                                                                md.commonMetadata.AutoExposureMode = (uint16_t)ptr->autoExposureMode;
                                                                md.commonMetadata.Gain = (uint16_t)ptr->manualGain;

                                                                //Common MD
                                                                md.commonMetadata.Timestamp = ptr->hwTimestamp;
                                                                md.commonMetadata.exposureTime = ptr->exposureTime;
                                                                md.commonMetadata.width = ptr->inputWidth;
                                                                md.commonMetadata.height = ptr->inputHeight;
                                                                // md.commonMetadata.frameId = ptr->frameCounter;
                                                                md.commonMetadata.CRC = ptr->crc32;

                                                                md.commonMetadata.Type = type;
                                                            }
                                                            if (type == SensorType::Color)
                                                            {
                                                                STMetaDataExtMipiRgb *ptr = static_cast<STMetaDataExtMipiRgb *>(metaDataBuffers[mdV4l2Buffer.index] + 16);

                                                                md.colorMetadata.BackLighCompensation = (uint16_t)ptr->backlight_Comp;
                                                                md.colorMetadata.Brightness = (uint16_t)ptr->brightness;
                                                                md.colorMetadata.Contrast = (uint16_t)ptr->contrast;
                                                                md.colorMetadata.Gamma = ptr->gamma;
                                                                md.colorMetadata.Hue = (uint16_t)ptr->hue;
                                                                md.colorMetadata.Saturation = (uint16_t)ptr->saturation;
                                                                md.colorMetadata.Sharpness = (uint16_t)ptr->sharpness;
                                                                md.colorMetadata.WhiteBalance = ptr->manual_WB;
                                                                md.colorMetadata.PowerLineFrequency = ptr->powerLineFrequency;
                                                                md.colorMetadata.LowLightCompensation = ptr->low_Light_comp;
                                                                md.colorMetadata.AutoWhiteBalanceTemp = ptr->auto_WB_Temp;

                                                                md.commonMetadata.manualExposure = ptr->manual_Exp;
                                                                md.commonMetadata.AutoExposureMode = (uint16_t)ptr->auto_Exp_Mode;
                                                                md.commonMetadata.Gain = (uint16_t)ptr->gain;

                                                                //Common MD
                                                                md.commonMetadata.Timestamp = ptr->hwTimestamp;
                                                                md.commonMetadata.exposureTime = ptr->manual_Exp;
                                                                md.commonMetadata.width = ptr->inputWidth;
                                                                md.commonMetadata.height = ptr->inputHeight;
                                                                md.commonMetadata.CRC = ptr->crc32;
                                                                md.commonMetadata.Type = type;
                                                            }

                                                            //print the raw data
                                                            // u_int* newPTR = static_cast<u_int*>(metaDataBuffers[mdV4l2Buffer.index] + 16);
                                                            // cout <<"========================================================================" <<endl;
                                                            // for (size_t i = 0; i < 68/4 - 4; ++i) {
                                                                
                                                            //     std::cout << std::setw(2) << std::setfill('0') << std::hex << newPTR[i] << " ";
                                                            //     //cout << hex << unsigned(newPTR[i]) << " ";
                                                            // }
                                                            // std::cout << std::dec << std::endl; // Reset to decimal after printing in hex
                                                            // cout <<"========================================================================" <<endl;
                                                            if(type == SensorType::Color || type == SensorType::Depth || type == SensorType::IR){
                                                                uint32_t crc = crc32buf(static_cast<uint8_t*>(metaDataBuffers[mdV4l2Buffer.index] + 16), 48/*sizeof(STMetaDataExtMipiDepthIR) - 4*/);
                                                                md.commonMetadata.CrcCorrectness = crc == md.commonMetadata.CRC;
                                                                if(md.commonMetadata.CrcCorrectness)
                                                                    cout << "CRC OK: " << std::hex << crc << " | " <<std::hex << md.commonMetadata.CRC << endl;
                                                                else
                                                                    cout<<"Crc Check Failed: " << std::hex << crc << " | " <<std::hex << md.commonMetadata.CRC << endl;                                                 
                                                            }
                                                        }


                                                        frame.frameMD = md;

                                                        // if this is an actual frame then call the callback functtion
                                                        if (frame.hwTimestamp == 0 || (metaFileOpened  && frame.frameMD.commonMetadata.Timestamp == 0))
                                                        {
                                                            Logger::getLogger().log("Sensor: " + name + ", Frame #" + to_string(frame.ID) + " with HW TimeStamp =0 was Dropped ", "Sensor", LOG_WARNING);
                                                        }
                                                        else
                                                        {
                                                            //Send the new created frame with the callback
                                                            (*FramesCallback)(frame);
                                                        }
                                                                                                                
                                                        //Free memory of the allocated buffer
                                                        if (copyFrameData)
                                                        {
                                                            free(frame.Buff);
                                                        }
                                                        if (ioctl(dataFileDescriptor, VIDIOC_QBUF, &V4l2Buffer) != 0)
                                                            Logger::getLogger().log(name + " Frame VIDIOC_QBUF Failed", "Sensor", LOG_ERROR);
                                                        if (metaFileOpened)
                                                        {
                                                            if (ioctl(metaFileDescriptor, VIDIOC_QBUF, &mdV4l2Buffer) != 0)
                                                                Logger::getLogger().log(name + " MD VIDIOC_QBUF Failed", "Sensor", LOG_ERROR);
                                                        }
                                                    }

                                                        //std::this_thread::sleep_for(std::chrono::milliseconds(100));

                                                }
                                                catch (...)
                                                {
                                                    Logger::getLogger().log(name + " Exception Thrown !!!", "Sensor",LOG_ERROR);
                                                }
                                               cameraBusy = false;
                                               Logger::getLogger().log(name + " Stream Thread terminated", "Sensor"); });
    }

    void Stop()
    {
        Logger::getLogger().log("Stopping " + name + " requested", "Sensor");
        stopRequested = true;

        // wait for the start thread to be terminated
        if (cameraBusy)
        {
            _t->join();
        }
        cout << "insideIOCTL= " << insideIOCTL << endl;
        while (insideIOCTL != 0)
        {
            cout << "insideIOCTL= " << insideIOCTL << endl;
            cout << GetName() << " waiting for ioctl counter: " << insideIOCTL << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        cout << "insideIOCTL= " << insideIOCTL << endl;

        try
        {
            // VIDIOC_STREAMOFF
            Logger::getLogger().log("Stopping " + name + " Streaming", "Sensor");
            int ret = ioctl(dataFileDescriptor, VIDIOC_STREAMOFF, &vType);
            if (ret == 0)
                Logger::getLogger().log(name + " - Streaming stopped", "Sensor");
            else
                Logger::getLogger().log("Failed to Stop " + name + " Streaming with return value of " + to_string(ret), "Sensor");

            if (metaFileOpened)
            {
                ret = ioctl(metaFileDescriptor, VIDIOC_STREAMOFF, &mdType);
                if (ret == 0)
                    Logger::getLogger().log(name + " - MD stopped", "Sensor");
                else
                    Logger::getLogger().log("Failed to Stop " + name + " MD with return value of " + to_string(ret), "Sensor");
            }
            // Unmap Buffers
            Logger::getLogger().log("unmapping " + name + " Stream buffers", "Sensor");
            for (int i = 0; i < framesBuffer.size(); i++)
            {
                // munmap(framesBuffer[i], lastProfile.GetBpp() * lastProfile.resolution.width * lastProfile.resolution.height);
                munmap(framesBuffer[i], lastProfile.GetSize());
                munmap(metaDataBuffers[i], 4096);
            }
            Logger::getLogger().log("unmapping " + name + " buffers Done", "Sensor");

            int bytes_per_line = 0;

            Logger::getLogger().log(GetName() + " Configuring Stride to : " + to_string(bytes_per_line), "Sensor");
            struct v4l2_control setStride;
            setStride.id = TEGRA_CAMERA_CID_VI_PREFERRED_STRIDE;
            setStride.value = bytes_per_line; // this should be 64 aligned and large enough for width * bpp, for example for Y8 848 set bytes_per_line to 896.
            ret = ioctl(dataFileDescriptor, VIDIOC_S_CTRL, &setStride);
            if (0 != ret)
            {
                Logger::getLogger().log(GetName() + " Failed to Configure Stride to : " + to_string(bytes_per_line), "Sensor", LOG_ERROR);
                throw std::runtime_error("Failed to set Fps");
            }
            else
                Logger::getLogger().log(GetName() + " Done Configuring Stride to : " + to_string(bytes_per_line), "Sensor");
        }
        catch (...)
        {
            Logger::getLogger().log("Stop() method of " + name + " Sensor was failed with exception", "Sensor");
        }
    }

    void Close()
    {
        // Close only the opened file descriptors
        if (dataFileOpened)
        {
            Logger::getLogger().log("Closing " + name + " stream file descriptor", "Sensor");
            close(dataFileDescriptor);
            dataFileOpened = false;
            dataFileDescriptor = 0;
            isClosed = true;
            Logger::getLogger().log(name + " stream file descriptor closed successfully", "Sensor");
        }
        if (metaFileOpened)
        {
            Logger::getLogger().log("Closing " + name + " MD file descriptor", "Sensor");
            close(metaFileDescriptor);
            metaFileOpened = false;
            metaFileDescriptor = 0;
            Logger::getLogger().log(name + " MD file descriptor closed successfully", "Sensor");
        }
    }

    uint32_t crc32buf(uint8_t *buf, int len)
    {
        uint32_t oldcrc32 = 0xFFFFFFFF;
        for (; len; --len, ++buf)
            oldcrc32 = UPDC32(*buf, oldcrc32);
        return ~oldcrc32;
    }
};

class Camera
{
private:
    std::vector<Sensor> sensors;
    string name;
    string serial;
    string fwVersion;

    string uintToVersion(uint32_t your_int)
    {
        std::stringstream stream;
        stream << std::hex << your_int;
        auto hexStr = stream.str();
        hexStr = ((hexStr.size() % 2 > 0) ? "0" : "") + hexStr; // padding with 0 if needed
        auto groupCount = hexStr.size() / 2 + ((hexStr.size() % 2 > 0) ? 1 : 0);
        string parts[groupCount];
        int j = groupCount;

        for (int i = hexStr.size() - 2; i >= 0; i -= 2)
        {
            j--;
            parts[j] = hexStr.substr(i, 2);
        }

        // generate the new int string (grouped)
        std::stringstream version;
        for (int i = 0; i < groupCount; i++)
        {
            // parse the str to int
            int tmp;
            std::stringstream ss;
            ss << std::hex << parts[i];
            ss >> tmp;
            version << tmp << ((i < groupCount - 1) ? "." : "");
        }
        return version.str();
    }

public:
    bool HWReset(int timeout = 10)
    {
        Sensor *depthSensor = GetDepthSensor();
        if (depthSensor->getIsClosed())
            depthSensor->Init(depthSensor->getType(), depthSensor->getOpenMetaD());
        string serial_number_before;
        string serial_number_after;

        for (int i = 0; i < 10; i++)
        {
            try
            {
                serial_number_before = CalcSerialNumber();
                break;
            }
            catch (const std::exception &e)
            {
            }
        }

        // Perform HW Reset

        HWMonitorCommand hmc = {0};

        hmc.dataSize = 0;
        hmc.opCode = 0x20; // HW reset opcode

        auto cR = SendHWMonitorCommand(hmc);
        if (cR.Result)
        {

            Logger::getLogger().log("Camera HW reset performed", "Camera");
        }
        else
        {
            // throw std::runtime_error("Failed to perform camera HW reset");
            Logger::getLogger().log("Camera HW reset Failed ", "Camera", LOG_ERROR);
            depthSensor->Close();
            return false;
        }

        bool cameraConnected = false;
        for (int i = 0; i < timeout; i++)
        {
            try
            {
                serial_number_after = CalcSerialNumber();
            }
            catch (const std::exception &e)
            {
            }
            if (serial_number_after != serial_number_before)
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            else
            {
                Logger::getLogger().log("Camera recognized after HW reset", "Camera");
                cameraConnected = true;
                break;
            }
        }
        if (!cameraConnected)
            Logger::getLogger().log("Failed to recognize the camera after HW reset", "Camera", LOG_ERROR);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        depthSensor->Close();
        return cameraConnected;
    }

    string CalcSerialNumber()
    {
        string serial_number;
        HWMonitorCommand hmc = {0};
        // GVD command
        hmc.dataSize = 0;
        hmc.opCode = 0x10; // GVD

        auto cR = SendHWMonitorCommand(hmc);
        if (cR.Result)
        {
            stringstream ss;
            ss << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[48]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[49]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[50]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[51]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[52]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[53]) << endl;
            serial_number = ss.str();
            serial_number.erase(std::remove(serial_number.begin(), serial_number.end(), '\n'), serial_number.end());
        }
        else
        {
            throw std::runtime_error("Failed to Read Serial number");
        }
        return serial_number;
    };
    string GetFwVersion()
    {
        return fwVersion;
    }

    string GetSerialNumber()
    {
        return serial;
    };

    void Init(bool openMD = true)
    {
        Logger::getLogger().log("Initializing camera sensors", "Camera");
        sensors.clear();
        // Try to add depth sensor
        Sensor depthSensor;
        if (depthSensor.Init(SensorType::Depth, openMD))
            sensors.push_back(depthSensor);

        // Try to add IR sensor
        Sensor irSensor;
        if (irSensor.Init(SensorType::IR, openMD))
            sensors.push_back(irSensor);

        // Try to add Color sensor
        Sensor colorSensor;
        if (colorSensor.Init(SensorType::Color, openMD))
        {
            sensors.push_back(colorSensor);
        }

        // Try to add IMU sensor
        Sensor imuSensor;
        if (imuSensor.Init(SensorType::IMU, openMD))
        {
            sensors.push_back(imuSensor);
        }

        // =========== Get FW Version and serial number =================
        uint32_t fwVersion_uint{0};
        struct v4l2_ext_control ctrl
        {
            0
        };
        ctrl.id = DS5_CAMERA_CID_FW_VERSION;
        ctrl.size = sizeof(fwVersion_uint);
        ctrl.p_u32 = &fwVersion_uint;

        struct v4l2_ext_controls ext
        {
            0
        };
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &ctrl;
        ext.count = 1;

        auto fd = depthSensor.GetFileDescriptor();

        int res = ioctl(fd, VIDIOC_G_EXT_CTRLS, &ext);
        if (res != 0)
            throw std::runtime_error("Failed to Read FW Version");

        fwVersion = uintToVersion(fwVersion_uint);

        // get Serial number

        HWMonitorCommand hmc = {0};

        // GVD command
        hmc.dataSize = 0;
        hmc.opCode = 0x10; // GVD

        auto cR = SendHWMonitorCommand(hmc);
        if (cR.Result)
        {
            stringstream ss;
            ss << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[48]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[49]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[50]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[51]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[52]) << std::hex << setfill('0') << setw(2) << unsigned(cR.Data[53]) << endl;
            serial = ss.str();
            serial.erase(std::remove(serial.begin(), serial.end(), '\n'), serial.end());
        }
        else
        {
            throw std::runtime_error("Failed to Read Serial number");
        }

        //=================================================================
    }

    vector<Sensor> GetSensors()
    {
        return sensors;
    }

    Sensor *GetDepthSensor()
    {
        for (int i = 0; i < sensors.size(); i++)
        {
            if (sensors[i].GetName() == "Depth Sensor")
                return &sensors[i];
        }
        throw std::runtime_error("Failed to get Depth Sensor ");
    }

    Sensor *GetIRSensor()
    {
        for (int i = 0; i < sensors.size(); i++)
        {
            if (sensors[i].GetName() == "IR Sensor")
                return &sensors[i];
        }
        throw std::runtime_error("Failed to get IR Sensor ");
    }

    Sensor *GetColorSensor()
    {
        for (int i = 0; i < sensors.size(); i++)
        {
            if (sensors[i].GetName() == "Color Sensor")
                return &sensors[i];
        }
        throw std::runtime_error("Failed to get Color Sensor ");
    }

    Sensor *GetIMUSensor()
    {
        for (int i = 0; i < sensors.size(); i++)
        {
            if (sensors[i].GetName() == "IMU Sensor")
                return &sensors[i];
        }
        throw std::runtime_error("Failed to get IMU Sensor ");
    }

    CommandResult SendHWMonitorCommand(HWMonitorCommand hmc)
    {
        CommandResult cR;

        // preapare the depth sensor where the HWMonitor command should be run with
        auto depthSensor = GetDepthSensor();
        bool wasClosed;
        if (depthSensor->getIsClosed())
            wasClosed = true;
        else
            wasClosed = false;
        if (wasClosed)
            depthSensor->Init(depthSensor->getType(), depthSensor->getOpenMetaD());
        int fd = depthSensor->GetFileDescriptor();

        // init the byte array of the hwmc
        int buffSize = 1028;
        uint8_t hwmc[buffSize]{0};

        struct v4l2_ext_control ctrl
        {
            0
        };
        ctrl.id = DS5_CAMERA_CID_HWMC;
        ctrl.size = sizeof(hwmc);
        ctrl.p_u8 = hwmc;

        struct v4l2_ext_controls ext
        {
            0
        };
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &ctrl;
        ext.count = 1;

        // prepare the command struct
        HWMC commandStruct{0};
        commandStruct.header = 0x14 + hmc.dataSize;
        commandStruct.magic_word = 0xCDAB;
        commandStruct.opcode = hmc.opCode;
        commandStruct.params[0] = hmc.parameter1;
        commandStruct.params[1] = hmc.parameter2;
        commandStruct.params[2] = hmc.parameter3;
        commandStruct.params[3] = hmc.parameter4;

        // prepare the buffer which should include the returned data
        uint8_t tempReturnedData[buffSize - (sizeof(struct HWMC) + 4)]{0};

        memcpy(hwmc, &commandStruct, sizeof(struct HWMC));

        // if command includes data, copy the data which receive by the function as a parameter to the hwmc
        if (hmc.dataSize > 0)
        {
            // memcpy(hwmc + sizeof(struct HWMC), &commandStruct + sizeof(struct HWMC), hmc.dataSize);
            memcpy(hwmc + sizeof(struct HWMC), hmc.data, hmc.dataSize);
        }

        // send the HWMonitor command and make sure that the ioctl is successfull
        if (0 != ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext))
        {
            cR.Result = false;
            if (wasClosed)
            {
                depthSensor->Close();
            }
            return cR;
        }

        // Check if command succeeded by making sure the returned opcode is equal to the sent one
        if (commandStruct.opcode == *(hwmc + sizeof(struct HWMC)))
        {
            cR.Result = true;
            memcpy(tempReturnedData, hwmc + sizeof(struct HWMC) + 4, sizeof(tempReturnedData));

            // parse the array to vector
            for (int i = 0; i < sizeof(tempReturnedData); i++)
            {
                cR.Data.push_back(tempReturnedData[i]);
            }
        }
        else
        {
            cR.Result = false;
        }
        if (wasClosed)
        {
            depthSensor->Close();
        }
        return cR;
    }

    int GetAsicTemperature()
    {
        HWMonitorCommand hmc = {0};

        hmc.dataSize = 0;
        hmc.opCode = 0x7A;
        hmc.parameter1 = 0x0;
        hmc.parameter2 = 0x0;
        auto cR = SendHWMonitorCommand(hmc);
        if (cR.Result)
            return cR.Data[0];
        else
        {
            Logger::getLogger().log("Failed to get Asic temperature from Camera", "Camera", LOG_ERROR);
            // throw std::runtime_error("Failed to get Asic temperature from Camera");
            return -1;
        }
    }
    int GetProjectorTemperature()
    {
        HWMonitorCommand hmc = {0};

        hmc.dataSize = 0;
        hmc.opCode = 0x2A;
        hmc.parameter1 = 0x0;
        hmc.parameter2 = 0x0;
        auto cR = SendHWMonitorCommand(hmc);
        if (cR.Result)
            return cR.Data[0];
        else
        {
            Logger::getLogger().log("Failed to get Projector temperature from Camera", "Camera", LOG_ERROR);
            // throw std::runtime_error("Failed to get Projector temperature from Camera");
            return -1;
        }
    }
    // For debugging
    void PrintBytes(uint8_t buff[], int len)
    {

        for (int i = 0; i < len; i++)
        {
            if (i % 16 == 0)
                cout << endl;

            cout << hex << unsigned(buff[i]) << " ";
        }
    }
};
