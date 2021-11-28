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
        case V4L2_PIX_FMT_Y8I:
            return "Y8I";
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
};

struct DepthMetadata
{
    double LaserPowerMode = 0;
    double ManualLaserPower = 0;
};

class Metadata
{
public:
    CommonMetadata commonMetadata;
    ColorMetadata colorMetadata;
    DepthMetadata depthMetadata;

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
        else if (name == "LaserPowerMode" )
            return depthMetadata.LaserPowerMode;
        else if (name == "ManualLaserPower")
            return depthMetadata.ManualLaserPower;
        else if (name == "AutoExposure")
            return commonMetadata.AutoExposure;
        else if (name == "AutoExposureMode")
            return commonMetadata.AutoExposureMode;
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

        //thread open_thread(open, __path,__oflag);
        thread open_thread([&]()
                           {
                               result = open(__path, __oflag);
                               Logger::getLogger().log("Open Sensor Succeeded");
                               isDone = true;
                           });

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
                //Kill the open thread
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

        case SensorType::Color:
            Logger::getLogger().log("Openning /dev/video2", "Sensor");
            videoNode = {"/dev/video2"};
            dataFileDescriptor = Open_timeout(videoNode.c_str(), O_RDWR, 1000);
            // dataFileDescriptor = open(videoNode.c_str(), O_RDWR);

            //  if (openMD)
            //  {
            // videoNode = {"/dev/video3"};
            // metaFileDescriptor = open(videoNode.c_str(), O_RDWR);
            // }
            name = "Color Sensor";
            dataFileOpened = dataFileDescriptor > 0;
            metaFileOpened = metaFileDescriptor > 0;
            isClosed = !dataFileOpened;

            Logger::getLogger().log("Init Sensor Color Done", "Sensor");
            return dataFileOpened;
        }
        return false;
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
        //sFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_Z16;
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

        //Set the fps
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
                                                   Logger::getLogger().log("Stream VIDIOC_STREAMON Filed", "Sensor", LOG_ERROR);
                                                   throw std::runtime_error("Failed to Start stream\n");
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
                                                       throw std::runtime_error("Failed to Start MD stream\n");
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
                                                   int ret = ioctl(dataFileDescriptor, VIDIOC_DQBUF, &V4l2Buffer);
                                                   ret = ioctl(metaFileDescriptor, VIDIOC_DQBUF, &mdV4l2Buffer);

                                                   //cout << "index = " << V4l2Buffer.index << endl;
                                                   //memcpy(frame.Buff, V4l2Buffer.m.userptr[V4l2Buffer.index].buff, framesBuffer[V4l2Buffer.index].lenght);
                                                   //  cout << "Index " << V4l2Buffer.index << endl;
                                                   //    int outfd = open("/home/nvidia/out.img", O_RDWR);
                                                   //    uint8_t *newBuffer =  (uint8_t *)framesBuffer[0];
                                                   //    write(outfd, newBuffer, V4l2Buffer.bytesused);
                                                   // cout<<"wite doneeeeeeeeeeeeeeeeeeeeeeeeeeeeee" << newBuffer << endl;
                                                   // close(outfd);

                                                   //Prepare the new Frame
                                                   Frame frame;

                                                   //copy the frame content
                                                   if (copyFrameData)
                                                   {
                                                       frame.Buff = (uint8_t *)malloc(V4l2Buffer.bytesused);
                                                       memcpy(frame.Buff, &V4l2Buffer.m.userptr, V4l2Buffer.bytesused);
                                                   }

                                                   frame.systemTimestamp = TimeUtils::getCurrentTimestamp();
                                                   frame.ID = V4l2Buffer.sequence;
                                                   //frame.streamType = V4l2Buffer.type;
                                                   frame.streamType = lastProfile.streamType;
                                                   frame.size = V4l2Buffer.bytesused;
                                                   frame.hwTimestamp = V4l2Buffer.timestamp.tv_usec;

                                                   //    cout << "FrameID: " << frame.ID << "\r" << flush;
                                                   //    cout << "FrameID: " << frame.ID << endl;

                                                   Metadata md;
                                                   if (metaFileOpened)
                                                   {
                                                       STMetaDataDepthYNormalMode *ptr = static_cast<STMetaDataDepthYNormalMode *>(
                                                           metaDataBuffers[mdV4l2Buffer.index]);

                                                       md.commonMetadata.Timestamp = ptr->captureStats.hwTimestamp;
                                                       md.commonMetadata.exposureTime = ptr->captureStats.ExposureTime;
                                                       md.commonMetadata.AutoExposureMode = ptr->intelDepthControl.autoExposureMode;
                                                       md.commonMetadata.Gain = ptr->intelDepthControl.manualGain;
                                                       md.commonMetadata.frameId = ptr->intelCaptureTiming.frameCounter;
                                                       md.commonMetadata.CRC = ptr->crc32;
                                                       // uint32_t crc = crc32buf(static_cast<uint8_t*>(metaDataBuffers[mdV4l2Buffer.index]), sizeof(STMetaDataDepthYNormalMode) - 4);
                                                   if (type==SensorType::Depth)
                                                   {
                                                       md.depthMetadata.LaserPowerMode = (uint16_t)ptr->intelDepthControl.projectorMode;
                                                       md.depthMetadata.ManualLaserPower = ptr->intelDepthControl.laserPower;
                                                   }
                                                    if (type==SensorType::Color)
                                                   {
                                                       md.colorMetadata.BackLighCompensation = 0;
                                                       md.colorMetadata.Brightness = 0;
                                                       md.colorMetadata.Contrast = 0;
                                                       md.colorMetadata.Gamma = 0;
                                                       md.colorMetadata.Hue = 0;
                                                       md.colorMetadata.Saturation = 0;
                                                       md.colorMetadata.Sharpness = 0;
                                                       md.colorMetadata.WhiteBalance = 0;
                                                   }
                                                   
                                                   }
                                                   frame.frameMD = md;

                                                   //Send the new created frame with the callback
                                                   (*FramesCallback)(frame);

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
                                               Logger::getLogger().log("Stream Thread terminated", "Sensor");
                                           });
    }

    void Stop()
    {
        stopRequested = true;

        //wait for the start thread to be terminated
        _t->join();

        //Unmap Buffers
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
        ret = ioctl(metaFileDescriptor, VIDIOC_STREAMOFF, &mdType);
        Logger::getLogger().log("Streaming stopped", "Sensor");
    }

    void Close()
    {
        //Close only the opened file descriptors
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

        //generate the new int string (grouped)
        std::stringstream version;
        for (int i = 0; i < groupCount; i++)
        {
            //parse the str to int
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
        //Try to add depth sensor
        Sensor depthSensor;
        if (depthSensor.Init(SensorType::Depth, openMD))
            sensors.push_back(depthSensor);

        //Try to add Color sensor
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
        // if (res != 0)
        //     throw std::runtime_error("Failed to Read FW Version");

        // fwVersion = uintToVersion(fwVersion_uint);
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

        //preapare the depth sensor where the HWMonitor command should be run with
        auto depthSensor = GetDepthSensor();
        int fd = depthSensor.GetFileDescriptor();

        //init the byte array of the hwmc
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

        //prepare the buffer which should include the returned data
        uint8_t tempReturnedData[buffSize - (sizeof(struct HWMC) + 4)]{0};

        memcpy(hwmc, &commandStruct, sizeof(struct HWMC));

        //if command includes data, copy the data which receive by the function as a parameter to the hwmc
        if (hmc.dataSize > 0)
        {
            memcpy(hwmc + sizeof(struct HWMC), &commandStruct + sizeof(struct HWMC), hmc.dataSize);
        }

        //send the HWMonitor command and make sure that the ioctl is successfull
        if (0 != ioctl(fd, VIDIOC_S_EXT_CTRLS, &ext))
        {
            cR.Result = false;
            return cR;
        }

        //Check if command succeeded by making sure the returned opcode is equal to the sent one
        if (commandStruct.opcode == *(hwmc + sizeof(struct HWMC)))
        {
            cR.Result = true;
            memcpy(tempReturnedData, hwmc + sizeof(struct HWMC) + 4, sizeof(tempReturnedData));

            //parse the array to vector
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
    //For debugging
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