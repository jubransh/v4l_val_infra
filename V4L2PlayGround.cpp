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

/*
TEST_F(TestBase, ColorStreamingTest){
    testDuration = 5;
    bool testStatus = true;
    SequentialFrameDropsMetric met;
    FramesArrivedMetric met2;

    FirstFrameDelayMetric met3;
    metrics.push_back(&met);
    metrics.push_back(&met2);
    metrics.push_back(&met3);

    Camera cam;
    cam.Init();
    Sensor  colorSensor= cam.GetColorSensor();
    vector<Profile> pR;

    vector<StreamType> types;
    types.push_back(StreamType::Color_Stream);
    vector<vector<Profile>> profiles = GetProfiles(types);

    for (int j = 0; j < profiles.size(); j++)
    {

        Logger::getLogger().log("=================================================","Run",LOG_INFO);
        Logger::getLogger().log("               ColorStreamingExample ","Run",LOG_INFO);
        Logger::getLogger().log("=================================================","Run",LOG_INFO);

        Logger::getLogger().log("Started Iteration: " + to_string(j),"Run");
        Logger::getLogger().log("Profile Used: " + profiles[j][0].GetText(),"Run");
        initFrameLists();

        pR.clear();
        pR.push_back(profiles[j][0]);
        setCurrentProfiles(pR);


        colorSensor.Configure(profiles[j][0]);
        long startTime = TimeUtils::getCurrentTimestamp();
        colorSensor.Start(AddFrame);

        std::this_thread::sleep_for(std::chrono::seconds(testDuration));

        colorSensor.Stop();
        colorSensor.Close();
        //std::this_thread::sleep_for(std::chrono::seconds(2));
        met.setParams(2);
        met2.setParams(5);
        met3.setParams(1000,startTime);

        bool result = CalcMetrics(j);
        if (!result)
            testStatus = false;
        Logger::getLogger().log("Iteration :" +to_string(j)+ " Done - Iteration Result: " + to_string(result),"Run");
        colorSensor.Close();
    }
    ASSERT_TRUE(testStatus);
}

TEST_F(TestBase, DepthStreamingTest)
{
    testDuration = 5;
    bool testStatus = true;
    SequentialFrameDropsMetric met;
    FramesArrivedMetric met2;

    FirstFrameDelayMetric met3;
    metrics.push_back(&met);
    metrics.push_back(&met2);
    metrics.push_back(&met3);

    Camera cam;
    cam.Init();
    Sensor depthSensor =cam.GetDepthSensor();
    /// Depth streams
    vector<StreamType> types;
    types.push_back(StreamType::Depth_Stream);
    vector<vector<Profile>> profiles = GetProfiles(types);
    vector<Profile> pR;
    for (int j = 0; j < profiles.size(); j++)
    {
        Logger::getLogger().log("=================================================","Run",LOG_INFO);
        Logger::getLogger().log("               DepthStreamingExample ","Run",LOG_INFO);
        Logger::getLogger().log("=================================================","Run",LOG_INFO);

        Logger::getLogger().log("Started Iteration: " + to_string(j),"Run");
        Logger::getLogger().log("Profile Used: " + profiles[j][0].GetText(),"Run");
        initFrameLists();

        pR.clear();
        pR.push_back(profiles[j][0]);
        setCurrentProfiles(pR);


        depthSensor.Configure(profiles[j][0]);
        long startTime = TimeUtils::getCurrentTimestamp();
        depthSensor.Start(AddFrame);

        std::this_thread::sleep_for(std::chrono::seconds(testDuration));

        depthSensor.Stop();
        depthSensor.Close();
        //std::this_thread::sleep_for(std::chrono::seconds(2));
        met.setParams(2);
        met2.setParams(5);
        met3.setParams(1000,startTime);

        bool result = CalcMetrics(j);
        if (!result)
            testStatus = false;
        Logger::getLogger().log("Iteration :" +to_string(j)+ " Done - Iteration Result: " + to_string(result),"Run");
        depthSensor.Close();

    }


    ASSERT_TRUE(testStatus);
}


TEST_F(TestBase, IRStreamingTest)
{
    testDuration = 5;
    bool testStatus = true;
    SequentialFrameDropsMetric met;
    FramesArrivedMetric met2;

    FirstFrameDelayMetric met3;
    metrics.push_back(&met);
    metrics.push_back(&met2);
    metrics.push_back(&met3);

    Camera cam;
    cam.Init();
    Sensor depthSensor =cam.GetDepthSensor();
    /// Depth streams
    vector<StreamType> types;
    types.push_back(StreamType::IR_Stream);
    vector<vector<Profile>> profiles = GetProfiles(types);
    vector<Profile> pR;
    for (int j = 0; j < profiles.size(); j++)
    {
        Logger::getLogger().log("=================================================","Run",LOG_INFO);
        Logger::getLogger().log("               IRStreamingExample ","Run",LOG_INFO);
        Logger::getLogger().log("=================================================","Run",LOG_INFO);

        Logger::getLogger().log("Started Iteration: " + to_string(j),"Run");
        Logger::getLogger().log("Profile Used: " + profiles[j][0].GetText(),"Run");
        initFrameLists();

        pR.clear();
        pR.push_back(profiles[j][0]);
        setCurrentProfiles(pR);


        depthSensor.Configure(profiles[j][0]);
        long startTime = TimeUtils::getCurrentTimestamp();
        depthSensor.Start(AddFrame);

        std::this_thread::sleep_for(std::chrono::seconds(testDuration));

        depthSensor.Stop();
        depthSensor.Close();
        //std::this_thread::sleep_for(std::chrono::seconds(2));
        met.setParams(2);
        met2.setParams(5);
        met3.setParams(1000,startTime);

        bool result = CalcMetrics(j);
        if (!result)
            testStatus = false;
        Logger::getLogger().log("Iteration :" +to_string(j)+ " Done - Iteration Result: " + to_string(result),"Run");
        depthSensor.Close();

    }


    ASSERT_TRUE(testStatus);
}



TEST_F(TestBase, MultiStreamingTest)
{
    testDuration = 5;
    bool testStatus = true;
    // SequentialFrameDropsMetric met;
    // FramesArrivedMetric met2;
    // FpsValidityMetric met4;
    // FrameSizeMetric met5;;
    // FrameDropIntervalMetric met6;
    FrameDropsPercentageMetric met7;
    IDCorrectnessMetric met8;

    // FirstFrameDelayMetric met3;
    // metrics.push_back(&met);
    // metrics.push_back(&met2);
    // metrics.push_back(&met4);
    // metrics.push_back(&met3);
    // metrics.push_back(&met5);
    // metrics.push_back(&met6);
    // metrics.push_back(&met7);
    metrics.push_back(&met8);

    Camera cam;
    cam.Init();
    Sensor depthSensor= cam.GetDepthSensor();
    Sensor colorSensor= cam.GetColorSensor();
    vector<StreamType> types;
    types.push_back(StreamType::Depth_Stream);
    types.push_back(StreamType::Color_Stream);
    vector<vector<Profile>> profiles = GetProfiles(types);

    //auto depthProfiles = GetProfiles(StreamType::Depth_Stream);
    //auto colorProfiles = GetProfiles(StreamType::Color_Stream);
    vector<Profile> pR;
    for (int j = 0; j < profiles.size(); j++)
    {

        Logger::getLogger().log("=================================================","Run",LOG_INFO);
        Logger::getLogger().log("               MultiStreamingExample ","Run",LOG_INFO);
        Logger::getLogger().log("=================================================","Run",LOG_INFO);

        Logger::getLogger().log("Started Iteration: " + to_string(j),"Run");
        Logger::getLogger().log("Depth Profiles Used: " + profiles[j][0].GetText(),"Run");
        Logger::getLogger().log("Color Profiles Used: " + profiles[j][1].GetText(),"Run");
        initFrameLists();

        pR.clear();
        pR.push_back(profiles[j][0]);
        pR.push_back(profiles[j][1]);
        setCurrentProfiles(pR);


        depthSensor.Configure(profiles[j][0]);
        colorSensor.Configure(profiles[j][1]);
        long startTime = TimeUtils::getCurrentTimestamp();
        depthSensor.Start(AddFrame);
        colorSensor.Start(AddFrame);

        std::this_thread::sleep_for(std::chrono::seconds(testDuration));

        depthSensor.Stop();
        depthSensor.Close();
        colorSensor.Stop();
        colorSensor.Close();
        //std::this_thread::sleep_for(std::chrono::seconds(2));
        // met.setParams(2);
        // met2.setParams(1,startTime, testDuration);
        // met3.setParams(1000,startTime);
        // met4.setParams(5);
        // met5.setParams(1);
        // met6.setParams(1);
        // met7.setParams(5);
        met8.setParams(5);

        bool result = CalcMetrics(j);
        if (!result)
            testStatus = false;
        Logger::getLogger().log("Iteration :" +to_string(j)+ " Done - Iteration Result: " + to_string(result),"Run");


    }

    ASSERT_TRUE(testStatus);
}

*/
TEST_F(TestBase, DepthControlTest)
{
    testDuration = 5;
    bool testStatus = true;
    SequentialFrameDropsMetric met;
    FramesArrivedMetric met2;
    FpsValidityMetric met4;
    FrameSizeMetric met5;
    ;
    FrameDropIntervalMetric met6;
    FrameDropsPercentageMetric met7;
    IDCorrectnessMetric met8;
    ControlLatencyMetric met9;
    MetaDataCorrectnessMetric met10;

    FirstFrameDelayMetric met3;
    metrics.push_back(&met);
    metrics.push_back(&met2);
    metrics.push_back(&met4);
    metrics.push_back(&met3);
    metrics.push_back(&met5);
    metrics.push_back(&met6);
    metrics.push_back(&met7);
    metrics.push_back(&met8);
    metrics.push_back(&met9);
    metrics.push_back(&met10);

    Camera cam;
    cam.Init();
    Sensor depthSensor = cam.GetDepthSensor();
    vector<StreamType> types;
    types.push_back(StreamType::Depth_Stream);
    vector<vector<Profile>> profiles = GetProfiles(types);

    // auto depthProfiles = GetProfiles(StreamType::Depth_Stream);
    // auto colorProfiles = GetProfiles(StreamType::Color_Stream);
    vector<Profile> pR;
    for (int j = 0; j < profiles.size(); j++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        Logger::getLogger().log("=================================================", "Run", LOG_INFO);
        Logger::getLogger().log("               MultiStreamingExample ", "Run", LOG_INFO);
        Logger::getLogger().log("=================================================", "Run", LOG_INFO);

        Logger::getLogger().log("Started Iteration: " + to_string(j), "Run");
        Logger::getLogger().log("Depth Profiles Used: " + profiles[j][0].GetText(), "Run");
        initFrameLists();

        pR.clear();
        pR.push_back(profiles[j][0]);

        setCurrentProfiles(pR);

        depthSensor.Configure(profiles[j][0]);
        bool res = depthSensor.SetControl(DS5_CAMERA_CID_MANUAL_LASER_POWER, 120);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        long startTime = TimeUtils::getCurrentTimestamp();

        depthSensor.Start(AddFrame);

        std::this_thread::sleep_for(std::chrono::seconds(testDuration / 2));
        long changeTime = TimeUtils::getCurrentTimestamp();
        res = depthSensor.SetControl(DS5_CAMERA_CID_MANUAL_LASER_POWER, 150);
        Logger::getLogger().log("Setting laser power control value was: ", "Run");
        Logger::getLogger().log((res) ? "Succeeded" : "Failed", "Run");

        std::this_thread::sleep_for(std::chrono::seconds(testDuration / 2));

        depthSensor.Stop();
        depthSensor.Close();
        // std::this_thread::sleep_for(std::chrono::seconds(2));
        met.setParams(2);
        met2.setParams(1, startTime, testDuration);
        met3.setParams(1000, startTime);
        met4.setParams(5);
        met5.setParams(1);
        met6.setParams(1);
        met7.setParams(5);
        met8.setParams(5);
        met9.setParams(5, changeTime, "ManualLaserPower", 150);
        met10.setParams(1);

        bool result = CalcMetrics(j);
        if (!result)
            testStatus = false;
        Logger::getLogger().log("Iteration :" + to_string(j) + " Done - Iteration Result: " + to_string(result), "Run");
    }

    ASSERT_TRUE(testStatus);
}

class V4L2BasicTest : public testing::Test
{

protected:
    void SetUp() override
    {
        string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();

        Logger::getLogger().configure("/home/nvidia/Desktop/Logs/logtest.log");
    }

    void TearDown() override
    {
    }
};

static double lastDepthTs = 0;
static double lastColorTs = 0;

// implemetnt the callback
void depthFrameArrived(Frame f)
{

    // frames.push_back(s);
    double currTs = f.frameMD.commonMetadata.Timestamp;
    cout << "Depth Frame #" << f.ID << " Arrived: @ " << currTs << endl;

    if (lastDepthTs == 0)
    {
        cout << "curr = " << currTs << endl;
        lastDepthTs = currTs;
        return;
    }

    long delta = currTs - lastDepthTs;
    lastDepthTs = currTs;

    cout << "Delta = " << delta << endl;
}

void colorFrameArrived(Frame f)
{
    // frames.push_back(s);
    double currTs = f.hwTimestamp;
    cout << "Color Frame #" << f.ID << " Arrived: @ " << f.hwTimestamp << endl;
    if (lastColorTs == 0)
    {
        cout << "curr = " << currTs << endl;
        lastColorTs = currTs;
        return;
    }

    long delta = currTs - lastColorTs;
    lastColorTs = currTs;

    cout << "Color Delta = " << delta << endl;
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
    cam.Init();
    auto depthSensor = cam.GetDepthSensor();
    auto colorSensor = cam.GetColorSensor();

    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 30;

    // Color Configuration
    Resolution cR = {0};
    cR.width = 640;
    cR.height = 480;
    Profile cP;
    cP.pixelFormat = V4L2_PIX_FMT_YUYV;
    cP.resolution = cR;
    cP.fps = 30;

    depthSensor.Configure(dP);
    colorSensor.Configure(cP);

    depthSensor.Start(depthFrameArrived);
    colorSensor.Start(colorFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    depthSensor.Stop();
    colorSensor.Stop();

    colorSensor.Close();
    depthSensor.Close();
}
