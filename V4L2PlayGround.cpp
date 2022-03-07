/*
 * Copyright © 2019 Intel Corporation. All rights reserved.
 *
 * The source code contained or described herein and all documents related to the source code
 * ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the
 * Material remains with Intel Corporation or its suppliers and licensors. The Material may
 * contain trade secrets and proprietary and confidential information of Intel Corporation
 * and its suppliers and licensors, and is protected by worldwide copyright and trade secret
 * laws and treaty provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way
 * without Intel’s prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual property right
 * is granted to or conferred upon you by disclosure or delivery of the Materials, either
 * expressly, by implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in writing.
 */

using namespace std;


class V4L2BasicTest : public testing::Test
{

protected:
    void SetUp() override
    {
        string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();

        // Logger::getLogger().configure("/home/nvidia/Desktop/Logs/logtest.log");
    }

    void TearDown() override
    {
    }
};

static double lastDepthTs = 0;
static double lastIRTs = 0;
static double lastColorTs = 0;

struct TestFrame
{
    string type;
    int id;
    long ts;
};

vector<TestFrame> depthFrames;
vector<TestFrame> irFrames;
vector<TestFrame> colorFrames;


// implemetnt the callback
void depthFrameArrived(Frame depthFrame)
{
    cout << "*" << endl;
    TestFrame f;
    f.id = depthFrame.ID;
    f.ts = depthFrame.hwTimestamp;
    f.type = "depth";

    depthFrames.push_back(f);

    // double currTs = f.systemTimestamp;
    // cout << "Depth Frame #" << f.ID << " Arrived: @ " << currTs << endl;
    // cout << endl << "Depth," << f.ID << "," << currTs;

    // if (lastDepthTs == 0)
    // {
    //     // cout << "curr = " << currTs << endl;
    //     lastDepthTs = currTs;
    //     return;
    // }

    // long delta = currTs - lastDepthTs;
    // lastDepthTs = currTs;
    // if (delta > (33 * 1.5))
    //     cout << "@@ Drop @@  Depth Delta for frame " << f.ID << " is  " << delta << " and TS is:" << currTs << " and Frame size is:" << f.size << endl;
    // // else
    //     //cout << "Depth Delta for frame " << f.ID << " is  " << delta << endl;
 
}

void irFrameArrived(Frame irFrame)
{
    cout << "*" << endl;
    TestFrame f;
    f.id = irFrame.ID;
    f.ts = irFrame.hwTimestamp;
    f.type = "ir";

    irFrames.push_back(f);

    // double currTs = f.systemTimestamp;
    // cout << "IR Frame #" << f.ID << " Arrived: @ " << currTs << endl;
    // cout << endl << "IR," << f.ID << "," << currTs;

    // if (lastIRTs == 0)
    // {
    //     // cout << "curr = " << currTs << endl;
    //     lastIRTs = currTs;
    //     return;
    // }

    // long delta = currTs - lastIRTs;
    // lastIRTs = currTs;
    // if (delta > (33 * 1.5))
    //     cout << "@@ Drop @@  IR Delta for frame " << f.ID << " is  " << delta << " and TS is:" << currTs << " and Frame size is:" << f.size << endl;
    // // else
    //     //cout << "IR Delta for frame " << f.ID << " is  " << delta << endl;
        
}

void colorFrameArrived(Frame colorFrame)
{
    cout << "*" << endl;
    TestFrame f;
    f.id = colorFrame.ID;
    f.ts = colorFrame.hwTimestamp;
    f.type = "color";

    colorFrames.push_back(f);
    
    // double currTs = f.systemTimestamp;
    //cout << "Color Frame #" << f.ID << " Arrived: @ " << f.hwTimestamp << endl;
    // cout << endl << "IR," << f.ID << "," << currTs;

    // if (lastColorTs == 0)
    // {
    //     // cout << "curr = " << currTs << endl;
    //     lastColorTs = currTs;
    //     return;
    // }

    // long delta = currTs - lastColorTs;
    // lastColorTs = currTs;
    // if (delta > (33 * 1.5))
    //     cout << "@@ Drop @@ Color Delta for frame " << f.ID << " is  " << delta << " and TS is:" << currTs << endl;
    // // else
    //     //cout << endl << "Color Delta for frame " << f.ID << " is  " << delta ;

}

// for debugging
void PrintBytes(vector<uint8_t> v)
{

    for (int i = 0; i < v.size(); i++)
    {
        if (i % 16 == 0)
            cout << endl;

        // cout << hex << buff[i]  << " " ;
        cout << hex << unsigned(v[i]) << " ";
        // printf("%c ", buff[i]);
    }
}

TEST_F(V4L2BasicTest, WriteHWMonitorCommandTest)
{
    Logger::getLogger().log("test started", "Main", LOG_INFO);
    cout << "=================================================" << endl;
    cout << "        write 2 bytes to runtime table " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // bytes to write (the data bytes array)
    uint8_t runtimeTable[] = {0x1, 0x2};

    // Write command (fwb)
    hmc.dataSize = 2;
    hmc.opCode = 0xa;
    hmc.parameter1 = 0x17b000;
    hmc.parameter2 = 0x2;
    hmc.data = runtimeTable;
    auto cR = cam.SendHWMonitorCommand(hmc);

    ASSERT_TRUE(cR.Result);
}

TEST_F(V4L2BasicTest, ReadHWMonitorCommandTest)
{
    Logger::getLogger().log("test started", "Main", LOG_INFO);
    cout << "=================================================" << endl;
    cout << "               Read First 100 bytes " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // Read command (frb)
    hmc.dataSize = 0;
    hmc.opCode = 0x9;
    hmc.parameter1 = 0x0;
    hmc.parameter2 = 0x100;
    auto cR = cam.SendHWMonitorCommand(hmc);

    ASSERT_TRUE(cR.Result);
    if (cR.Result)
        PrintBytes(cR.Data);
}

TEST_F(V4L2BasicTest, GetLaserInfoCommand)
{
    cout << "=================================================" << endl;
    cout << "                  GPWM command " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // GPWM command
    hmc.dataSize = 0;
    hmc.opCode = 0x19;
    auto cR = cam.SendHWMonitorCommand(hmc);

    ASSERT_TRUE(cR.Result);
    if (cR.Result)
    {
        int minLaserVal = cR.Data[0] | cR.Data[1] << 8;
        int maxLaserVal = cR.Data[2] | cR.Data[3] << 8;
        int laserStepsNo = cR.Data[4] | cR.Data[5] << 8;
        cout << "Min laser value: " << minLaserVal << endl;
        cout << "Max laser value: " << maxLaserVal << endl;
        cout << "Laser number of steps: " << laserStepsNo << endl;
    }
}

TEST_F(V4L2BasicTest, SetGetControlsExample)
{
    cout << "=================================================" << endl;
    cout << "               SetGetControlsExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();
    auto depthSensor = cam.GetDepthSensor();
    auto colorSensor = cam.GetColorSensor();

    cout << "=================================================" << endl;
    cout << "Camera FW: " << cam.GetFwVersion() << endl;
    cout << "=================================================" << endl;

    cout << "=================================================" << endl;
    // Print the sensor supported resolutions of depth sensor
    cout << "Supported Depth resolutions are: " << endl;
    auto sR = depthSensor.GetSupportedResolutions();
    for (int i = 0; i < sR.size(); i++)
    {
        cout << sR[i].width << "x" << sR[i].height << endl;
    }
    cout << "=================================================" << endl;

    cout << "=================================================" << endl;
    // Print the sensor supported resolutions of color sensor
    cout << "Supported Color resolutions are: " << endl;
    sR = colorSensor.GetSupportedResolutions();
    for (int i = 0; i < sR.size(); i++)
    {
        cout << sR[i].width << "x" << sR[i].height << endl;
    }
    cout << "=================================================" << endl;

    cout << "=================================================" << endl;
    // Print the sensor supported Formats of depth sensor
    cout << "Supported Depth Formats are: " << endl;
    vector<uint32_t> fmt = depthSensor.GetSupportedFormats();
    for (int i = 0; i < fmt.size(); i++)
    {
        cout << fmt[i] << endl;
    }
    cout << "=================================================" << endl;

    cout << "=================================================" << endl;
    // Print the sensor supported Formats of color sensor
    cout << "Supported Color Formats are: " << endl;
    fmt = colorSensor.GetSupportedFormats();
    for (int i = 0; i < fmt.size(); i++)
    {
        cout << fmt[i] << endl;
    }
    cout << "=================================================" << endl;

    // Get Laser Power
    auto val = depthSensor.GetControl(DS5_CAMERA_CID_MANUAL_LASER_POWER);
    cout << "Curr laser power control value is: " << val << endl;

    // Set Laser power
    auto res = depthSensor.SetControl(DS5_CAMERA_CID_MANUAL_LASER_POWER, 150);
    cout << "Setting laser power control value was: " << ((res) ? "Succeeded" : "Failed") << endl;

    // Get Laser Power Again
    val = depthSensor.GetControl(DS5_CAMERA_CID_MANUAL_LASER_POWER);
    cout << "Curr laser power control value is: " << val << endl;

    cout << endl;
}

TEST_F(V4L2BasicTest, SaveImagesDepthStreamingExample)
{
    cout << "=================================================" << endl;
    cout << "               DepthStreamingExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();
    Sensor depthSensor = cam.GetDepthSensor();
    depthSensor.copyFrameData = true;
    // depthSensor.copyFrameData = true;

    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 60;
    dP.streamType = StreamType::Depth_Stream;

    depthSensor.Configure(dP);
    depthSensor.Start(depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    depthSensor.Stop();
    depthSensor.Close();
}

TEST_F(V4L2BasicTest, SaveImagesIRStreamingExample)
{
    cout << "=================================================" << endl;
    cout << "               IRStreamingExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init(false);
    auto irSensor = cam.GetIRSensor();
    irSensor.copyFrameData = true;

    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Y8;
    dP.resolution = r;
    dP.fps = 30;
    dP.streamType = StreamType::IR_Stream;

    irSensor.Configure(dP);

    irSensor.Start(depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    irSensor.Stop();

    irSensor.Close();
}

TEST_F(V4L2BasicTest, SaveImagesColorStreamingExample)
{
    cout << "=================================================" << endl;
    cout << "               ColorStreamingExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();
    auto colorSensor = cam.GetColorSensor();
    colorSensor.copyFrameData = true;

    // Color Configuration
    Resolution cR = {0};
    cR.width = 640;
    cR.height = 480;
    Profile cP;
    cP.pixelFormat = V4L2_PIX_FMT_YUYV;
    cP.resolution = cR;
    cP.fps = 30;
    cP.streamType = StreamType::Color_Stream;

    colorSensor.Configure(cP);

    colorSensor.Start(colorFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    colorSensor.Stop();

    colorSensor.Close();
}

TEST_F(V4L2BasicTest, DepthStreamingExample)
{
    cout << "=================================================" << endl;
    cout << "               DepthStreamingExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();
    auto depthSensor = cam.GetDepthSensor();
    // depthSensor.copyFrameData = true;

    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 30;
    dP.streamType = StreamType::Depth_Stream;

    depthSensor.Configure(dP);
    depthSensor.Start(depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    depthSensor.Stop();
    depthSensor.Close();
}

TEST_F(V4L2BasicTest, IRStreamingExample)
{
    cout << "=================================================" << endl;
    cout << "               IRStreamingExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init(false);
    auto depthSensor = cam.GetDepthSensor();

    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Y8;
    dP.resolution = r;
    dP.fps = 30;
    dP.streamType = StreamType::IR_Stream;

    depthSensor.Configure(dP);

    depthSensor.Start(depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    depthSensor.Stop();

    depthSensor.Close();
}

TEST_F(V4L2BasicTest, ColorStreamingExample)
{
    cout << "=================================================" << endl;
    cout << "               ColorStreamingExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();
    auto colorSensor = cam.GetColorSensor();

    // Color Configuration
    Resolution cR = {0};
    cR.width = 640;
    cR.height = 480;
    Profile cP;
    cP.pixelFormat = V4L2_PIX_FMT_YUYV;
    cP.resolution = cR;
    cP.fps = 30;
    cP.streamType = StreamType::Color_Stream;

    colorSensor.Configure(cP);

    colorSensor.Start(colorFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    colorSensor.Stop();

    colorSensor.Close();
}

TEST_F(V4L2BasicTest, MultiStreamingExample)
{
    cout << "=================================================" << endl;
    cout << "               MultiStreamingExample " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init(false);
    // cam.Init(false);
    auto depthSensor = cam.GetDepthSensor();
    auto colorSensor = cam.GetColorSensor();
    auto irSensor = cam.GetIRSensor();

    // Depth Configuration
    Resolution r = {0};
    r.width = 1280;
    r.height = 270;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 30;

    // ir Configuration
    Resolution IRr = {0};
    IRr.width = 1280;
    IRr.height = 270;
    Profile iP;
    iP.pixelFormat = V4L2_PIX_FMT_Y8;
    iP.resolution = r;
    iP.fps = 30; 

    // Color Configuration
    Resolution cR = {0};
    cR.width = 1280;
    cR.height = 720;
    Profile cP;
    cP.pixelFormat = V4L2_PIX_FMT_YUYV;
    cP.resolution = cR;
    cP.fps = 30;

    colorSensor.Configure(cP);
    depthSensor.Configure(dP);
    irSensor.Configure(iP);


    colorSensor.Start(colorFrameArrived);
    depthSensor.Start(depthFrameArrived);
    irSensor.Start(irFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    colorSensor.Stop();
    depthSensor.Stop();
    irSensor.Stop();

    colorSensor.Close();
    depthSensor.Close();
    irSensor.Close();

    //print frames data
    for (int i = 0; i < depthFrames.size(); i++)
    {
        cout << depthFrames[i].type << "," << depthFrames[i].id << "," << depthFrames[i].ts << endl;
    }
    for (int i = 0; i < irFrames.size(); i++)
    {
        cout << irFrames[i].type << "," << irFrames[i].id << "," << irFrames[i].ts << endl;
    }
    for (int i = 0; i < colorFrames.size(); i++)
    {
        cout << colorFrames[i].type << "," << colorFrames[i].id << "," << colorFrames[i].ts << endl;
    }        

}

TEST_F(V4L2BasicTest, StreamTestPattern)
{
    cout << "=================================================" << endl;
    cout << "         Enable Test Pattern command " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // GPWM command
    hmc.dataSize = 0;
    hmc.opCode = 0x80;  // CUMSOM_CMD
    hmc.parameter1 = 7; // Test Pattern
    hmc.parameter2 = 1; // On

    auto depthSensor = cam.GetDepthSensor();
    auto colorSensor = cam.GetColorSensor();

    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 60;

    // Color Configuration
    Resolution cR = {0};
    cR.width = 640;
    cR.height = 480;
    Profile cP;
    cP.pixelFormat = V4L2_PIX_FMT_YUYV;
    cP.resolution = cR;
    cP.fps = 30;

    depthSensor.Configure(dP);
    // colorSensor.Configure(cP);

    depthSensor.Start(depthFrameArrived);
    // colorSensor.Start(colorFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));
    cout << "Enabled Pattern" << endl;
    auto cResult = cam.SendHWMonitorCommand(hmc);

    std::this_thread::sleep_for(std::chrono::seconds(4));
    hmc.parameter2 = 0; // Off
    cout << "Disabled Pattern" << endl;
    cResult = cam.SendHWMonitorCommand(hmc);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    depthSensor.Stop();
    // colorSensor.Stop();

    // colorSensor.Close();
    depthSensor.Close();

    // ASSERT_TRUE(cResult.Result);
    // if (cResult.Result){
    //     cout << "Test Pattern Enabled" << endl;
    // }
    // else{
    //     cout << "Disabling Test Pattern Failed" << endl;
    // }
}

TEST_F(V4L2BasicTest, EnableTestPattern)
{
    cout << "=================================================" << endl;
    cout << "         Enable Test Pattern command " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // GPWM command
    hmc.dataSize = 0;
    hmc.opCode = 0x80;  // CUMSOM_CMD
    hmc.parameter1 = 7; // Test Pattern
    hmc.parameter2 = 1; // On

    auto cR = cam.SendHWMonitorCommand(hmc);

    ASSERT_TRUE(cR.Result);
    if (cR.Result)
    {
        cout << "Test Pattern Enabled" << endl;
    }
    else
    {
        cout << "Disabling Test Pattern Failed" << endl;
    }
}

TEST_F(V4L2BasicTest, DisableTestPattern)
{
    cout << "=================================================" << endl;
    cout << "         Disable Test Pattern command " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // GPWM command
    hmc.dataSize = 0;
    hmc.opCode = 0x80;  // CUMSOM_CMD
    hmc.parameter1 = 7; // Test Pattern
    hmc.parameter2 = 0; // On

    auto cR = cam.SendHWMonitorCommand(hmc);

    ASSERT_TRUE(cR.Result);
    if (cR.Result)
    {
        cout << "Test Pattern Disabled" << endl;
    }
    else
    {
        cout << "Disabling Test Pattern Failed" << endl;
    }
}

TEST_F(V4L2BasicTest, StartStopTestPattern)
{
    cout << "=================================================" << endl;
    cout << "         Disable Test Pattern command " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // GPWM command
    hmc.dataSize = 0;
    hmc.opCode = 0x80;  // CUMSOM_CMD
    hmc.parameter1 = 7; // Test Pattern

    CommandResult cR;

    for (int i = 0; i < 10; i++)
    {
        hmc.parameter2 = 1; // On
        cR = cam.SendHWMonitorCommand(hmc);

        // ASSERT_TRUE(cR.Result);
        if (cR.Result)
        {
            cout << "Test Pattern Disabled" << endl;
        }
        else
        {
            cout << "Disabling Test Pattern Failed" << endl;
        }

        cout << "Waiting 1 second" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        hmc.parameter2 = 0; // Off
        cR = cam.SendHWMonitorCommand(hmc);

        // ASSERT_TRUE(cR.Result);
        if (cR.Result)
        {
            cout << "Test Pattern Disabled" << endl;
        }
        else
        {
            cout << "Disabling Test Pattern Failed" << endl;
        }
    }
}

TEST_F(V4L2BasicTest, GVD)
{
    cout << "=================================================" << endl;
    cout << "         Disable Test Pattern command " << endl;
    cout << "=================================================" << endl;

    Camera cam;
    cam.Init();

    HWMonitorCommand hmc = {0};

    // GVD command
    hmc.dataSize = 0;
    hmc.opCode = 0x10; // GVD

    auto cR = cam.SendHWMonitorCommand(hmc);

    ASSERT_TRUE(cR.Result);
    if (cR.Result)
    {
        cout << "GVD" << endl;
    }
}
