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

// #include <opencv2/opencv.hpp>
using namespace std;
// using namespace cv;

void write_to_file(string filePath, uint8_t *ptr, size_t len)
{
    try
    {
        cout << "first byte: " << int(ptr[0]) << endl;
        cout << "second byte: " << int(ptr[1]) << endl;
        cout << "3rd byte: " << int(ptr[2]) << endl;
        cout << "last byte: " << int(ptr[len -1]) << endl;
        cout << "data len: " << len << endl;

        ofstream fp;
        fp.open(filePath.c_str(), ios :: binary );
        fp.write((char *)ptr, len);
        fp.close();
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << endl;
    }
}

class V4L2BasicTest2 : public testing::Test
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

static double lastDepthTs2 = 0;
static double lastColorTs2 = 0;

// implemetnt the callback
void save_depthFrameArrived(Frame f)
{
    if (f.ID ==10)
    {
        cout << " Depth Delta for frame = " << f.ID << endl;
        write_to_file(FileUtils::getHomeDir() + "/Logs/image" + to_string(f.streamType) + ".bin", f.Buff, f.size);
    }

    // frames.push_back(s);
    double currTs = f.frameMD.commonMetadata.Timestamp;
    // cout << "Depth Frame #" << f.ID << " Arrived: @ " << currTs << endl;

    if (lastDepthTs2 == 0)
    {
        // cout << "curr = " << currTs << endl;
        lastDepthTs2 = currTs;
        return;
    }

    long delta = currTs - lastDepthTs2;
    lastDepthTs2 = currTs;
    if (delta > 33333 * 1.05 / 2 || delta < 33333 * 0.95 / 2)
        cout << " Depth Delta for frame = " << f.ID << " is  " << delta << " and TS is:" << currTs << " and Frame size is:" << f.size << endl;
}

void save_colorFrameArrived(Frame f)
{
    if (f.ID == 10)
    {
        write_to_file(FileUtils::getHomeDir() + "/Logs" + to_string(f.hwTimestamp) + "Color_image.bin", f.Buff, f.size);
        cout << " Color Delta for frame = " << f.ID << endl;
    }
    // frames.push_back(s);
    double currTs = f.hwTimestamp;
    // cout << "Color Frame #" << f.ID << " Arrived: @ " << f.hwTimestamp << endl;
    if (lastColorTs2 == 0)
    {
        // cout << "curr = " << currTs << endl;
        lastColorTs2 = currTs;
        return;
    }

    long delta = currTs - lastColorTs2;
    lastColorTs2 = currTs;
    if (delta > 33333 * 1.05 || delta < 33333 * 0.95)
        cout << " Color Delta for frame = " << f.ID << " is  " << delta << " and TS is:" << currTs << endl;
}

TEST_F(V4L2BasicTest2, SaveImagesDepthStreamingExample)
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
    depthSensor.Start(save_depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    depthSensor.Stop();
    depthSensor.Close();
}

TEST_F(V4L2BasicTest2, SaveImagesIRStreamingExample)
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

    irSensor.Start(save_depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    irSensor.Stop();

    irSensor.Close();
}

TEST_F(V4L2BasicTest2, SaveImagesColorStreamingExample)
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

    colorSensor.Start(save_colorFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    colorSensor.Stop();

    colorSensor.Close();
}