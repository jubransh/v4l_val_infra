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
    Color_Stream
};
enum SensorType
{
    Depth,
    IR,
    Color
};

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
        case V4L2_PIX_FMT_Y8:
            return "Y8";
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

        default:
            return "";
            break;
        }
    }
    double GetSize()
    {
        int bpp;
        switch (pixelFormat)
        {
        case V4L2_PIX_FMT_Y8:
            bpp = 1;
            break;

        default:
            bpp = 2;
            break;
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
    double CRC = 0;
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
    }

    double getMetaDataByString(string name)
    {
        if (name == "frameId")
            return commonMetadata.frameId;
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
};

class Sensor
{
private:
    bool isClosed = true;
    std::shared_ptr<std::thread> _t;
    bool cameraBusy = false;
    int dataFileDescriptor = 0;
    int metaFileDescriptor = 0;

    bool metaFileOpened = false;
    bool dataFileOpened = false;
    bool openMetaD = true;

    string name;
    SensorType type;
    bool stopRequested = false;
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

        Logger::getLogger().log("Requesting buffers " + string((0 == ret) ? "Succeeded" : "Failed"), "Sensor", LOG_INFO);
        Logger::getLogger().log("Requested buffers number: " + to_string(v4L2ReqBufferrs.count), "Sensor", LOG_INFO);

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

            if (openMD)
            {
                Logger::getLogger().log("Openning /dev/video5", "Sensor");
                videoNode = {"/dev/video5"};
                metaFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
                // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);
            }
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
            return false;
        }
    }

    double GetControl(__u32 controlId)
    {
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
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &control;
        ext.count = 1;

        // get Control
        ext.controls = &control;
        ext.count = 1;
        int ret = ioctl(dataFileDescriptor, VIDIOC_G_EXT_CTRLS, &ext);

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
        ext.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
        ext.controls = &control;
        ext.count = 1;
        int ret = ioctl(dataFileDescriptor, VIDIOC_S_EXT_CTRLS, &ext);
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
        if (isClosed)
            Init(type, openMetaD);
        // Set Stream profile
        struct v4l2_format sFormat = {0};
        sFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        // sFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_Z16;
        sFormat.fmt.pix.pixelformat = p.pixelFormat;
        sFormat.fmt.pix.width = p.resolution.width;
        sFormat.fmt.pix.height = p.resolution.height;
        int ret = ioctl(dataFileDescriptor, VIDIOC_S_FMT, &sFormat);
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
        Logger::getLogger().log("Done configuring Sensor:" + GetName() + " with Profile: " + p.GetText(), "Sensor");
    }

    void Start(void (*FramesCallback)(Frame f))
    {
        Logger::getLogger().log("Starting Sensor: " + GetName(), "Sensor");

        cameraBusy = true;

        stopRequested = false;
        _t = std::make_shared<std::thread>([this, FramesCallback]()
                                           {
                                               // request buffers
                                               requestBuffers(dataFileDescriptor, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP, NUMBER_OF_BUFFERS);

                                               if (metaFileOpened)
                                                   requestBuffers(metaFileDescriptor, V4L2_BUF_TYPE_META_CAPTURE, V4L2_MEMORY_MMAP, NUMBER_OF_BUFFERS);

                                               for (int i = 0; i < framesBuffer.size(); ++i)
                                               {
                                                   framesBuffer[i] = queryMapQueueBuf(dataFileDescriptor,
                                                                                      V4L2_BUF_TYPE_VIDEO_CAPTURE,
                                                                                      V4L2_MEMORY_MMAP,
                                                                                      i,
                                                                                      lastProfile.GetBpp() * lastProfile.resolution.width * lastProfile.resolution.height);
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
                                                   Logger::getLogger().log("Stream VIDIOC_STREAMON Failed - "+name, "Sensor", LOG_ERROR);
                                                   throw std::runtime_error("Failed to Start stream - "+name+"\n");
                                               }
                                               else
                                               {
                                                   Logger::getLogger().log("Stream VIDIOC_STREAMON Done", "Sensor");
                                               }

                                               if (metaFileOpened)
                                               {
                                                   if (ioctl(metaFileDescriptor, VIDIOC_STREAMON, &mdType) != 0)
                                                   {
                                                       Logger::getLogger().log("Stream VIDIOC_STREAMON Failed", "Sensor", LOG_ERROR);
                                                       throw std::runtime_error("Failed to Start MD stream - "+name+"\n");
                                                   }
                                                   else
                                                   {
                                                       Logger::getLogger().log("Metadata VIDIOC_STREAMON Done", "Sensor");
                                                   }
                                               }

                                               long buffIndex = 0;
                                               while (!stopRequested)
                                               {
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
                                                   int res = ioctl(dataFileDescriptor, VIDIOC_DQBUF, &V4l2Buffer);
                                                   if(res != 0)
                                                        Logger::getLogger().log(name + " Frame VIDIOC_DQBUF Failed on result: " + to_string(res), "Sensor");
                                                   
                                                   if (metaFileOpened)
                                                   {
                                                        res = ioctl(metaFileDescriptor, VIDIOC_DQBUF, &mdV4l2Buffer);
                                                        if(res != 0)
                                                            Logger::getLogger().log(name + " MD VIDIOC_DQBUF Failed on result: " + to_string(res), "Sensor");
                                                   }
                                            
                                                   //Prepare the new Frame
                                                   Frame frame;

                                                   //copy the frame content
                                                   if (copyFrameData)
                                                   {
                                                       frame.Buff = (uint8_t *)malloc(V4l2Buffer.bytesused);
                                                       memcpy(frame.Buff, framesBuffer[V4l2Buffer.index], V4l2Buffer.bytesused);
                                                   }

                                                   frame.systemTimestamp = TimeUtils::getCurrentTimestamp();
                                                   frame.ID = V4l2Buffer.sequence;
                                                   frame.streamType = lastProfile.streamType;
                                                   frame.size = V4l2Buffer.bytesused;
                                                   frame.hwTimestamp = V4l2Buffer.timestamp.tv_usec;

                                                   Metadata md;                                                                                         


                                                   if (metaFileOpened)
                                                   {
                                                       md.commonMetadata.frameId=mdV4l2Buffer.sequence;
                                                       if (type == SensorType::Depth or type== SensorType::IR)
                                                       {
                                                            STMetaDataExtMipiDepthIR *ptr = static_cast<STMetaDataExtMipiDepthIR*>(metaDataBuffers[mdV4l2Buffer.index] + 16);

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
                                                            STMetaDataExtMipiRgb *ptr = static_cast<STMetaDataExtMipiRgb*>(metaDataBuffers[mdV4l2Buffer.index] + 16);

                                                            md.colorMetadata.BackLighCompensation = (uint16_t)ptr->backlight_Comp;
                                                            md.colorMetadata.Brightness = (uint16_t)ptr->brightness;
                                                            md.colorMetadata.Contrast = (uint16_t)ptr->contrast;
                                                            md.colorMetadata.Gamma = ptr->gamma;
                                                            md.colorMetadata.Hue = (uint16_t)ptr->hue;
                                                            md.colorMetadata.Saturation = (uint16_t)ptr->saturation;
                                                            md.colorMetadata.Sharpness =(uint16_t)ptr->sharpness;
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

                                                    //    // uint32_t crc = crc32buf(static_cast<uint8_t*>(metaDataBuffers[mdV4l2Buffer.index]), sizeof(STMetaDataDepthYNormalMode) - 4);

                                                   }
                                                   frame.frameMD = md;

                                                   // if this is an actual frame then call the callback functtion
                                                   if (!metaFileOpened || (frame.hwTimestamp!=0 && frame.frameMD.commonMetadata.Timestamp!=0))
                                                   {
                                                       //Send the new created frame with the callback
                                                       (*FramesCallback)(frame);
                                                   }
                                                   else
                                                   {
                                                       Logger::getLogger().log("Sensor: "+ name +", Frame #"+ to_string(frame.ID)+" with HW TimeStamp =0 was Dropped ", "Sensor",LOG_WARNING);
                                                   }
                                                   //Free memory of the allocated buffer
                                                   if (copyFrameData)
                                                   {
                                                       free(frame.Buff);
                                                   }
                                                    
                                                   //Return the buffer to the V4L
                                                   ioctl(dataFileDescriptor, VIDIOC_QBUF, &V4l2Buffer);
                                                   ioctl(metaFileDescriptor, VIDIOC_QBUF, &mdV4l2Buffer);
                                               }
                                               //    cout << endl;
                                               cameraBusy = false;
                                               Logger::getLogger().log("Stream Thread terminated", "Sensor"); });
    }

    void Stop()
    {
        stopRequested = true;

        // wait for the start thread to be terminated
        _t->join();

        // Unmap Buffers
        Logger::getLogger().log("unmapping buffers", "Sensor");
        for (int i = 0; i < framesBuffer.size(); i++)
        {
            munmap(framesBuffer[i], lastProfile.GetBpp() * lastProfile.resolution.width * lastProfile.resolution.height);
            munmap(metaDataBuffers[i], 4096);
        }
        Logger::getLogger().log("unmapping buffers Done", "Sensor");
        
        // VIDIOC_STREAMOFF
        Logger::getLogger().log("Stopping Streaming", "Sensor");
        int ret = ioctl(dataFileDescriptor, VIDIOC_STREAMOFF, &vType);
        if(ret == 0)
            Logger::getLogger().log(name + " - Streaming stopped", "Sensor");
        else
            Logger::getLogger().log("Failed to Stop " + name + " Streaming with return value of " + to_string(ret), "Sensor");


        if (metaFileOpened)
        {
            ret = ioctl(metaFileDescriptor, VIDIOC_STREAMOFF, &mdType);
            if(ret == 0)
                Logger::getLogger().log(name + " - MD stopped", "Sensor");
            else
                Logger::getLogger().log("Failed to Stop " + name + " MD with return value of " + to_string(ret), "Sensor");

        }
    }

    void Close()
    {
        // Close only the opened file descriptors
        if (dataFileOpened)
        {
            close(dataFileDescriptor);
            dataFileOpened = false;
            dataFileDescriptor = 0;
            isClosed = true;
        }
        if (metaFileOpened)
        {
            close(metaFileDescriptor);
            metaFileOpened = false;
            metaFileDescriptor = 0;
        }
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
            ss << std::hex<< setfill('0') << setw(2)<< unsigned(cR.Data[48]) << std::hex<< setfill('0') << setw(2)<< unsigned(cR.Data[49]) << std::hex<< setfill('0') << setw(2)<< unsigned(cR.Data[50])<<std::hex<< setfill('0') << setw(2)<< unsigned(cR.Data[51])<< std::hex<< setfill('0') << setw(2)<< unsigned(cR.Data[52])<< std::hex<< setfill('0') << setw(2)<< unsigned(cR.Data[53])<< endl;
            serial =  ss.str();
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

    Sensor GetDepthSensor()
    {
        for (int i = 0; i < sensors.size(); i++)
        {
            if (sensors[i].GetName() == "Depth Sensor")
                return sensors[i];
        }
        throw std::runtime_error("Failed to get Depth Sensor ");
    }

    Sensor GetIRSensor()
    {
        for (int i = 0; i < sensors.size(); i++)
        {
            if (sensors[i].GetName() == "IR Sensor")
                return sensors[i];
        }
        throw std::runtime_error("Failed to get IR Sensor ");
    }

    Sensor GetColorSensor()
    {
        for (int i = 0; i < sensors.size(); i++)
        {
            if (sensors[i].GetName() == "Color Sensor")
                return sensors[i];
        }
        throw std::runtime_error("Failed to get Color Sensor ");
    }

    CommandResult SendHWMonitorCommand(HWMonitorCommand hmc)
    {
        CommandResult cR;

        // preapare the depth sensor where the HWMonitor command should be run with
        auto depthSensor = GetDepthSensor();
        int fd = depthSensor.GetFileDescriptor();

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
            memcpy(hwmc + sizeof(struct HWMC), &commandStruct + sizeof(struct HWMC), hmc.dataSize);
        }

        // send the HWMonitor command and make sure that the ioctl is successfull
        if (0 != ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext))
        {
            cR.Result = false;
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
            throw std::runtime_error("Failed to get Asic temperature from Camera");
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
            throw std::runtime_error("Failed to get Projector temperature from Camera");
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
