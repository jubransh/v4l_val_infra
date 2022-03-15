// #include <ctime>
// #include <sys/mman.h>
// #include "MetaData.h"

using namespace std;
// #include <gtest/gtest.h>
// #include <fcntl.h>
// #include <sys/ioctl.h>
// #include <unistd.h>
// #include <linux/videodev2.h>
// #include <linux/v4l2-subdev.h>
// #include <errno.h>
// #include <cstdint>
// #include <vector>
// #include <algorithm>
// #include <array>
// #include <thread> // std::this_thread::sleep_for
// #include <chrono> // std::chrono::seconds
// #include <thread>
// #include "infra/TestInfra.cpp"
// #include "infra/cg.cpp"

class ControlsTest : public TestBase
{
public:
    void configure(int StreamDuration)
    {
        Logger::getLogger().log("Configuring stream duration to: " + to_string(StreamDuration), "Test", LOG_INFO);
        testDuration = StreamDuration;
    }
    int getMaxAllowedExposure(int fps, StreamType streamType)
    {
        if (streamType == StreamType::Depth_Stream || streamType == StreamType::IR_Stream)
        {
            switch (fps)
            {
            case 90:
                return 100;
                break;
            case 60:
                return 155;
                break;
            case 30:
                return 322;
                break;
            case 15:
                return 655;
                break;
            case 5:
                return 1650;
                break;
            }
        }
        else if (streamType == StreamType::Color_Stream)
        {
            switch (fps)
            {
            case 90:
                return 78;
                break;
            case 60:
                return 156;
                break;
            case 30:
                return 312;
                break;
            case 15:
                return 625;
                break;
            case 5:
                return 1250;
                break;
            }
        }
    }
    void run(StreamType stream, string controlName)
    {
        try
        {
        string name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        Logger::getLogger().log("               " + name + " Controls while Streaming Test ", "Test", LOG_INFO);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        bool res;
        int maxAllowedtExposure;
        string failedIterations = "Test Failed in Iterations: ";
        ControlConf cntrl;
        cntrl = ControlsGenerator::get_control_conf(controlName);
        ControlLatencyMetric cntrlMetric;
        FpsValidityMetric fpsMetric;
        FrameDropIntervalMetric frmIntervalMetric;
        FrameDropsPercentageMetric frmPercMetric;
        SequentialFrameDropsMetric seqFrmMetric;
        IDCorrectnessMetric idMetric;
        FrameSizeMetric frmSizeMetric;
        MetaDataCorrectnessMetric met_md_cor;

        metrics.push_back(&cntrlMetric);
        metrics.push_back(&fpsMetric);
        metrics.push_back(&frmIntervalMetric);
        metrics.push_back(&frmPercMetric);
        metrics.push_back(&seqFrmMetric);
        metrics.push_back(&idMetric);
        metrics.push_back(&frmSizeMetric);
        metrics.push_back(&met_md_cor);

        Sensor depthSensor = cam.GetDepthSensor();
        Sensor irSensor = cam.GetIRSensor();
        Sensor colorSensor = cam.GetColorSensor();
        vector<Profile> profiles = GetControlProfiles(stream);

        vector<Profile> pR;
        bool DepthUsed = false;
        bool ColorUsed = false;
        bool IRUsed = false;
        if (stream == StreamType::Depth_Stream)
            DepthUsed = true;
        else if (stream == StreamType::IR_Stream)
            IRUsed = true;
        else if (stream == StreamType::Color_Stream)
            ColorUsed = true;

        if (DepthUsed)
        {
            if (controlName == "Gain" || controlName == "Exposure")
            {
                Logger::getLogger().log("Disabling Depth AutoExposure ", "Test");
                res = depthSensor.SetControl(V4L2_CID_EXPOSURE_AUTO, 0);
                Logger::getLogger().log("Disable Depth AutoExposure " + (string)(res ? "Passed" : "Failed"), "Test");
            }
            if (controlName == "LaserPower")
            {
                Logger::getLogger().log("Enabling Laser Power Mode ", "Test");
                res = depthSensor.SetControl(DS5_CAMERA_CID_LASER_POWER, 1);
                Logger::getLogger().log("Enable Laser Power Mode:" + (string)(res ? "Passed" : "Failed"), "Test");
            }
            Logger::getLogger().log("Initializing Control: " + cntrl._controlName + " To Last value in list: " + to_string(cntrl._values[cntrl._values.size() - 1]), "Test");
            res = depthSensor.SetControl(cntrl._controlID, cntrl._values[cntrl._values.size() - 1]);
        }
        else if (IRUsed)
        {
            if (controlName == "Gain" || controlName == "Exposure")
            {
                Logger::getLogger().log("Disabling IR AutoExposure ", "Test");
                res = irSensor.SetControl(V4L2_CID_EXPOSURE_AUTO, 0);
                Logger::getLogger().log("Disable IR AutoExposure " + (string)(res ? "Passed" : "Failed"), "Test");
            }
            if (controlName == "LaserPower")
            {
                Logger::getLogger().log("Enabling Laser Power Mode ", "Test");
                res = irSensor.SetControl(DS5_CAMERA_CID_LASER_POWER, 1);
                Logger::getLogger().log("Enable Laser Power Mode:" + (string)(res ? "Passed" : "Failed"), "Test");
            }
            Logger::getLogger().log("Initializing Control: " + cntrl._controlName + " To Last value in list: " + to_string(cntrl._values[cntrl._values.size() - 1]), "Test");
            res = irSensor.SetControl(cntrl._controlID, cntrl._values[cntrl._values.size() - 1]);
        }
        else if (ColorUsed)
        {
            if (controlName == "Gain" || controlName == "Exposure")
            {
                Logger::getLogger().log("Disabling AutoExposure ", "Test");
                res = colorSensor.SetControl(V4L2_CID_EXPOSURE_AUTO, 0);
                Logger::getLogger().log("Disable AutoExposure " + (string)(res ? "Passed" : "Failed"), "Test");
            }
            Logger::getLogger().log("Initializing Control: " + cntrl._controlName + " To Last value in list: " + to_string(cntrl._values[cntrl._values.size() - 1]), "Test");

            res = colorSensor.SetControl(cntrl._controlID, cntrl._values[cntrl._values.size() - 1]);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        Logger::getLogger().log("Initializing Control: " + (string)(res ? "Passed" : "Failed"), "Test");

        for (int j = 0; j < profiles.size(); j++)
        {
            if (DepthUsed)
            {
                if (controlName == "Gain")
                {
                    maxAllowedtExposure = getMaxAllowedExposure(profiles[j].fps, StreamType::Depth_Stream);
                    Logger::getLogger().log("Setting depth exposure value to: " + to_string(maxAllowedtExposure), "Test");
                    res = depthSensor.SetControl(V4L2_CID_EXPOSURE_ABSOLUTE, maxAllowedtExposure);
                    Logger::getLogger().log("Setting depth exposure value " + (string)(res ? "Passed" : "Failed"), "Test");
                }
            }
            else if (IRUsed)
            {
                if (controlName == "Gain")
                {
                    maxAllowedtExposure = getMaxAllowedExposure(profiles[j].fps, StreamType::Depth_Stream);
                    Logger::getLogger().log("Setting IR exposure value to: " + to_string(maxAllowedtExposure), "Test");
                    res = irSensor.SetControl(V4L2_CID_EXPOSURE_ABSOLUTE, maxAllowedtExposure);
                    Logger::getLogger().log("Setting IR exposure value " + (string)(res ? "Passed" : "Failed"), "Test");
                }
            }
            else if (ColorUsed)
            {
                if (controlName == "Gain")
                {
                    maxAllowedtExposure = getMaxAllowedExposure(profiles[j].fps, StreamType::Color_Stream);
                    Logger::getLogger().log("Setting color exposure value to: " + to_string(maxAllowedtExposure), "Test");
                    res = colorSensor.SetControl(V4L2_CID_EXPOSURE_ABSOLUTE, maxAllowedtExposure);
                    Logger::getLogger().log("Setting color exposure value " + (string)(res ? "Passed" : "Failed"), "Test");
                }
            }

            Logger::getLogger().log("Sleeping for 3 seconds", "Test");
            std::this_thread::sleep_for(std::chrono::seconds(3));
            pR.clear();
            if (stream == StreamType::Depth_Stream)
            {
                Logger::getLogger().log("Depth Profile Used: " + profiles[j].GetText(), "Test");
                depthSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
            else if (stream == StreamType::IR_Stream)
            {
                Logger::getLogger().log("IR Profile Used: " + profiles[j].GetText(), "Test");
                irSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
            else if (stream == StreamType::Color_Stream)
            {
                Logger::getLogger().log("Color Profile Used: " + profiles[j].GetText(), "Test");
                colorSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
            setCurrentProfiles(pR);

            long startTime = TimeUtils::getCurrentTimestamp();
            if (DepthUsed)
            {
                depthSensor.Start(AddFrame);
            }
            if (IRUsed)
            {
                irSensor.Start(AddFrame);
            }
            if (ColorUsed)
            {
                colorSensor.Start(AddFrame);
            }
            for (int i = 0; i < cntrl._values.size(); i++)
            {
                initFrameLists();
                // collectFrames = true;
                if (IRUsed)
                    ir_collectFrames = true;
                else if (DepthUsed)
                    depth_collectFrames = true;
                else if (ColorUsed)
                    color_collectFrames = true;
                Logger::getLogger().log("Started Iteration: " + to_string(j * cntrl._values.size() + i), "Test");
                Logger::getLogger().log("Sleep before setting control: " + cntrl._controlName + " for " + to_string(testDuration / 2) + " seconds", "Test");
                std::this_thread::sleep_for(std::chrono::seconds(testDuration / 2));
                Logger::getLogger().log("Setting Control: " + cntrl._controlName + " to Value: " + to_string(cntrl._values[i]), "Test");
                long changeTime = TimeUtils::getCurrentTimestamp();
                if (DepthUsed || IRUsed)
                    res = depthSensor.SetControl(cntrl._controlID, cntrl._values[i]);
                if (IRUsed)
                    res = irSensor.SetControl(cntrl._controlID, cntrl._values[i]);
                if (ColorUsed)
                    res = colorSensor.SetControl(cntrl._controlID, cntrl._values[i]);

                Logger::getLogger().log("Setting Control: " + (res) ? "Passed" : "Failed", "Test");
                Logger::getLogger().log("Sleep after setting control: " + cntrl._controlName + " for " + to_string(testDuration / 2) + " seconds", "Test");
                std::this_thread::sleep_for(std::chrono::seconds(testDuration / 2));

                // collectFrames = false;
                if (IRUsed)
                    ir_collectFrames = false;
                else if (DepthUsed)
                    depth_collectFrames = false;
                else if (ColorUsed)
                    color_collectFrames = false;

                if (!(cntrl._controlName == "Exposure" || cntrl._controlName == "Color_Exposure"))
                {
                    /*
                    fpsMetric.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity());
                    frmIntervalMetric.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval());
                    frmPercMetric.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage());
                    seqFrmMetric.setParams(MetricDefaultTolerances::get_tolerance_SequentialFrameDrops());
                    idMetric.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness());
                    cntrlMetric.setParams(MetricDefaultTolerances::get_tolerance_ControlLatency(), changeTime, cntrl._mDName, cntrl._values[i]);
                    */
                    double prevValue;
                    if (i == 0)
                        prevValue = cntrl._values[cntrl._values.size() - 1];
                    else
                        prevValue = cntrl._values[i - 1];

                    fpsMetric.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity(), cntrl._values[i], changeTime, cntrl._mDName, cntrl._values[i]);
                    frmIntervalMetric.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval(), cntrl._values[i], changeTime, cntrl._mDName, cntrl._values[i]);
                    frmPercMetric.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage(), cntrl._values[i], changeTime, cntrl._mDName, cntrl._values[i]);
                    seqFrmMetric.setParams(MetricDefaultTolerances::get_tolerance_SequentialFrameDrops(), cntrl._values[i], changeTime, cntrl._mDName, cntrl._values[i]);
                    idMetric.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness(), cntrl._values[i], changeTime, cntrl._mDName, cntrl._values[i]);
                    cntrlMetric.setParams(MetricDefaultTolerances::get_tolerance_ControlLatency(), changeTime, cntrl._mDName, cntrl._values[i], prevValue);
                    met_md_cor.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness(), cntrl._values[i], changeTime, cntrl._mDName, cntrl._values[i]);
                }
                else
                {
                    double prevExposure;
                    if (i == 0)
                        prevExposure = cntrl._values[cntrl._values.size() - 1];
                    else
                        prevExposure = cntrl._values[i - 1];

                    fpsMetric.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity(), cntrl._values[i] * 100, changeTime, cntrl._mDName, cntrl._values[i] * 100);
                    frmIntervalMetric.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval(), cntrl._values[i] * 100, changeTime, cntrl._mDName, cntrl._values[i] * 100);
                    frmPercMetric.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage(), cntrl._values[i] * 100, changeTime, cntrl._mDName, cntrl._values[i] * 100);
                    seqFrmMetric.setParams(MetricDefaultTolerances::get_tolerance_SequentialFrameDrops(), cntrl._values[i] * 100, changeTime, cntrl._mDName, cntrl._values[i] * 100);
                    idMetric.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness(), cntrl._values[i] * 100, changeTime, cntrl._mDName, cntrl._values[i] * 100);
                    cntrlMetric.setParams(MetricDefaultTolerances::get_tolerance_ControlLatency(), changeTime, cntrl._mDName, cntrl._values[i], prevExposure);
                    met_md_cor.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness(), cntrl._values[i] * 100, changeTime, cntrl._mDName, cntrl._values[i] * 100);
                }

                frmSizeMetric.setParams(MetricDefaultTolerances::get_tolerance_FrameSize());

                bool result = CalcMetrics(j * cntrl._values.size() + i);
                if (!result)
                {
                    testStatus = false;
                    failedIterations += to_string(j * cntrl._values.size() + i) + ", ";
                }

                Logger::getLogger().log("Iteration :" + to_string(j * cntrl._values.size() + i) + " Done - Iteration Result: " + ((result) ? "Pass" : "Fail"), "Run");
            }
            if (DepthUsed)
            {
                depthSensor.Stop();
                depthSensor.Close();
            }
            if (IRUsed)
            {
                irSensor.Stop();
                irSensor.Close();
            }
            if (ColorUsed)
            {
                colorSensor.Stop();
                colorSensor.Close();
            }
        }
        Logger::getLogger().log("Test Summary:", "Run");
        Logger::getLogger().log(testStatus ? "Pass" : "Fail", "Run");
        if (!testStatus)
        {
            // for (int i; i<failedIterations.size();i++)
            // {
            //     text = text + to_string(failedIterations[i]);
            //     if (i!=failedIterations.size()-1)
            //         text= text+", ";
            // }
            Logger::getLogger().log(failedIterations, "Run");
        }
        }
        catch(...)
        {
            Logger::getLogger().log("Exeption cought", "Run", LOG_ERROR);
        }

        ASSERT_TRUE(testStatus);
    }
};

////////////////////////////////////////////////////////////////////////////
// Depth Control Tests
TEST_F(ControlsTest, Depth_Gain)
{
    configure(10);

    run(StreamType::Depth_Stream, "Gain");
}
TEST_F(ControlsTest, Depth_Exposure)
{
    configure(10);
    // IgnoreMetric("ID Correctness", StreamType::Depth_Stream);
    // IgnoreMetric("Frame drops interval", StreamType::Depth_Stream);
    // IgnoreMetric("Sequential frame drops", StreamType::Depth_Stream);
    // IgnoreMetric("FPS Validity", StreamType::Depth_Stream);
    run(StreamType::Depth_Stream, "Exposure");
}
TEST_F(ControlsTest, Depth_LaserPower)
{
    configure(10);
    run(StreamType::Depth_Stream, "LaserPower");
}

TEST_F(ControlsTest, Depth_LaserPowerMode)
{
    configure(10);
    run(StreamType::Depth_Stream, "LaserPowerMode");
}
////////////////////////////////////////////////////////////////////////////
// IR Control Tests

TEST_F(ControlsTest, IR_Gain)
{
    configure(10);

    run(StreamType::IR_Stream, "Gain");
}
TEST_F(ControlsTest, IR_Exposure)
{
    configure(10);
    // IgnoreMetric("ID Correctness", StreamType::Depth_Stream);
    // IgnoreMetric("Frame drops interval", StreamType::Depth_Stream);
    // IgnoreMetric("Sequential frame drops", StreamType::Depth_Stream);
    // IgnoreMetric("FPS Validity", StreamType::Depth_Stream);
    run(StreamType::IR_Stream, "Exposure");
}
TEST_F(ControlsTest, IR_LaserPower)
{
    configure(10);
    run(StreamType::IR_Stream, "LaserPower");
}

TEST_F(ControlsTest, IR_LaserPowerMode)
{
    configure(10);
    run(StreamType::IR_Stream, "LaserPowerMode");
}
// ////////////////////////////////////////////////////////////////////////////
// // Color Controls - Still not ready
// TEST_F(ControlsTest, Color_BackLighCompensation)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "BackLighCompensation");
// }

// TEST_F(ControlsTest, Color_Brightness)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Brightness");
// }

// TEST_F(ControlsTest, Color_Contrast)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Contrast");
// }

// TEST_F(ControlsTest, Color_Exposure)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Color_Exposure");
// }

// TEST_F(ControlsTest, Color_Gain)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Color_Gain");
// }

// TEST_F(ControlsTest, Color_Gamma)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Gamma");
// }

// TEST_F(ControlsTest, Color_Hue)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Hue");
// }

// TEST_F(ControlsTest, Color_Saturation)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Saturation");
// }

// TEST_F(ControlsTest, Color_Sharpness)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "Sharpness");
// }

// TEST_F(ControlsTest, Color_WhiteBalance)
// {
//     configure(10);
//     run(StreamType::Color_Stream, "WhiteBalance");
// }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ControlsSetGetTest : public TestBase
{
public:
    void configure()
    {
        // testDuration = StreamDuration;
    }
    void run(StreamType stream, string controlName)
    {
        string name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        Logger::getLogger().log("               " + name + " Controls Set/Get Test ", "Test", LOG_INFO);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        bool res;
        string failedIterations = "Test Failed in Iterations: ";
        ControlConf cntrl;
        cntrl = ControlsGenerator::get_control_conf(controlName);

        Sensor depthSensor = cam.GetDepthSensor();
        Sensor irSensor = cam.GetIRSensor();
        Sensor colorSensor = cam.GetColorSensor();

        bool DepthUsed = false;
        bool ColorUsed = false;
        bool IRUsed = false;
        if (stream == StreamType::Depth_Stream)
            DepthUsed = true;
        else if (stream == StreamType::IR_Stream)
            IRUsed = true;
        else if (stream == StreamType::Color_Stream)
            ColorUsed = true;
        if (DepthUsed)
        {
            if (controlName == "Gain" || controlName == "Exposure")
            {
                Logger::getLogger().log("Disabling Depth AutoExposure ", "Test");
                res = depthSensor.SetControl(V4L2_CID_EXPOSURE_AUTO, 0);
                Logger::getLogger().log("Disable Depth AutoExposure " + (string)(res ? "Passed" : "Failed"), "Test");
            }
            if (controlName == "LaserPower")
            {
                Logger::getLogger().log("Enabling Depth Laser Power Mode ", "Test");
                res = depthSensor.SetControl(DS5_CAMERA_CID_LASER_POWER, 1);
                Logger::getLogger().log("Enable Depth Laser Power Mode:" + (string)(res ? "Passed" : "Failed"), "Test");
            }
            Logger::getLogger().log("Initializing Depth Control: " + cntrl._controlName + " To Last value in list: " + to_string(cntrl._values[cntrl._values.size() - 1]), "Test");
            res = depthSensor.SetControl(cntrl._controlID, cntrl._values[cntrl._values.size() - 1]);
        }
        else if (IRUsed)
        {
            if (controlName == "Gain" || controlName == "Exposure")
            {
                Logger::getLogger().log("Disabling IR AutoExposure ", "Test");
                res = irSensor.SetControl(V4L2_CID_EXPOSURE_AUTO, 0);
                Logger::getLogger().log("Disable IR AutoExposure " + (string)(res ? "Passed" : "Failed"), "Test");
            }
            if (controlName == "LaserPower")
            {
                Logger::getLogger().log("Enabling IR Laser Power Mode ", "Test");
                res = irSensor.SetControl(DS5_CAMERA_CID_LASER_POWER, 1);
                Logger::getLogger().log("Enable IR Laser Power Mode:" + (string)(res ? "Passed" : "Failed"), "Test");
            }
            Logger::getLogger().log("Initializing IR Control: " + cntrl._controlName + " To Last value in list: " + to_string(cntrl._values[cntrl._values.size() - 1]), "Test");
            res = irSensor.SetControl(cntrl._controlID, cntrl._values[cntrl._values.size() - 1]);
        }
        else if (ColorUsed)
        {
            if (controlName == "Gain" || controlName == "Exposure")
            {
                Logger::getLogger().log("Disabling AutoExposure ", "Test");
                res = colorSensor.SetControl(V4L2_CID_EXPOSURE_AUTO, 0);
                Logger::getLogger().log("Disable AutoExposure " + (string)(res ? "Passed" : "Failed"), "Test");
            }

            Logger::getLogger().log("Initializing Control: " + cntrl._controlName + " To Last value in list: " + to_string(cntrl._values[cntrl._values.size() - 1]), "Test");

            res = colorSensor.SetControl(cntrl._controlID, cntrl._values[cntrl._values.size() - 1]);
        }
        Logger::getLogger().log("Initializing Control: " + (string)(res ? "Passed" : "Failed"), "Test");
        bool setRes;
        for (int j = 0; j < cntrl._values.size(); j++)
        {
            Logger::getLogger().log("Started Iteration: " + to_string(j), "Test");
            Logger::getLogger().log("Setting Control: " + cntrl._controlName + " to Value: " + to_string(cntrl._values[j]), "Test");
            if (DepthUsed)
                setRes = depthSensor.SetControl(cntrl._controlID, cntrl._values[j]);
            else if (IRUsed)
                setRes = irSensor.SetControl(cntrl._controlID, cntrl._values[j]);
            else if (ColorUsed)
                setRes = colorSensor.SetControl(cntrl._controlID, cntrl._values[j]);
            Logger::getLogger().log("Setting Control: " + (string)(setRes ? "Passed" : "Failed"), "Test");

            // std::this_thread::sleep_for(std::chrono::seconds(1));

            Logger::getLogger().log("Getting Control: " + cntrl._controlName + " Value", "Test");
            double currValue;
            if (DepthUsed)
                currValue = depthSensor.GetControl(cntrl._controlID);
            else if (IRUsed)
                currValue = irSensor.GetControl(cntrl._controlID);
            else if (ColorUsed)
                currValue = colorSensor.GetControl(cntrl._controlID);
            MetricResult result;
            string iterationStatus;
            iterations+=1;
            if (currValue == cntrl._values[j])
            {
                result.result = true;
                iterationStatus = "Pass";
            }
            else
            {
                result.result = false;
                iterationStatus = "Fail";
                failediter+=1;
            }
            string streamName = "";
            if (stream == StreamType::Depth_Stream)
                streamName = "Depth";
            else if (stream == StreamType::IR_Stream)
                streamName = "IR";
            else if (stream == StreamType::Color_Stream)
                streamName = "Color";

            result.remarks = "Control Name: " + cntrl._controlName + "\nSet Value: " + to_string(cntrl._values[j]) + "\nSet Status: " + ((setRes) ? "Pass" : "Fail") + "\nGet Value: " + to_string(currValue) + "\nMetric result: " + ((result.result) ? "Pass" : "Fail");
            string iRes = to_string(j) + ",,," + streamName + ",Set/Get," + ((result.result) ? "Pass" : "Fail") + ",\"" + result.remarks + "\",";
            AppendIterationResultCVS(iRes + iterationStatus);
            if (!result.result)
            {
                testStatus = false;
                failedIterations += to_string(j) + ", ";
            }

            Logger::getLogger().log("Iteration :" + to_string(j) + " Done - Iteration Result: " + ((result.result) ? "Pass" : "Fail"), "Run");
        }

        Logger::getLogger().log("Test Summary:", "Run");
        Logger::getLogger().log(testStatus ? "Pass" : "Fail", "Run");
        if (!testStatus)
        {
            // for (int i; i<failedIterations.size();i++)
            // {
            //     text = text + to_string(failedIterations[i]);
            //     if (i!=failedIterations.size()-1)
            //         text= text+", ";
            // }
            Logger::getLogger().log(failedIterations, "Run");
        }

        ASSERT_TRUE(testStatus);
    }
};

////////////////////////////////////////////////////////////////////////////
// Depth Set Get tests
TEST_F(ControlsSetGetTest, Depth_Gain_Set_Get)
{
    // configure(10);
    run(StreamType::Depth_Stream, "Gain");
}
TEST_F(ControlsSetGetTest, Depth_Exposure_Set_Get)
{
    // configure(10);
    run(StreamType::Depth_Stream, "Exposure");
}
TEST_F(ControlsSetGetTest, Depth_LaserPower_Set_Get)
{
    // configure(10);
    run(StreamType::Depth_Stream, "LaserPower");
}

TEST_F(ControlsSetGetTest, Depth_LaserPowerMode_Set_Get)
{
    // configure(10);
    run(StreamType::Depth_Stream, "LaserPowerMode");
}
/*
////////////////////////////////////////////////////////////////////////////
// IR Set Get tests
TEST_F(ControlsSetGetTest, IR_Gain_Set_Get)
{
    // configure(10);
    run(StreamType::IR_Stream, "Gain");
}
TEST_F(ControlsSetGetTest, IR_Exposure_Set_Get)
{
    // configure(10);
    run(StreamType::IR_Stream, "Exposure");
}
TEST_F(ControlsSetGetTest, IR_LaserPower_Set_Get)
{
    // configure(10);
    run(StreamType::IR_Stream, "LaserPower");
}

TEST_F(ControlsSetGetTest, IR_LaserPowerMode_Set_Get)
{
    // configure(10);
    run(StreamType::IR_Stream, "LaserPowerMode");
}
*/
///////////////////////////////////////////////////////////////////////////////
///  Color Set Get tests - Still not ready

// TEST_F(ControlsSetGetTest, Color_BackLighCompensation_Set_Get)
// {
//     run(StreamType::Color_Stream, "BackLighCompensation");
// }

// TEST_F(ControlsSetGetTest, Color_Brightness_Set_Get)
// {
//     run(StreamType::Color_Stream, "Brightness");
// }

// TEST_F(ControlsSetGetTest, Color_Contrast_Set_Get)
// {
//     run(StreamType::Color_Stream, "Contrast");
// }

// TEST_F(ControlsSetGetTest, Color_Exposure_Set_Get)
// {
//     run(StreamType::Color_Stream, "Color_Exposure");
// }

// TEST_F(ControlsSetGetTest, Color_Gain_Set_Get)
// {
//     run(StreamType::Color_Stream, "Color_Gain");
// }

// TEST_F(ControlsSetGetTest, Color_Gamma_Set_Get)
// {
//     run(StreamType::Color_Stream, "Gamma");
// }

// TEST_F(ControlsSetGetTest, Color_Hue_Set_Get)
// {
//     run(StreamType::Color_Stream, "Hue");
// }

// TEST_F(ControlsSetGetTest, Color_Saturation_Set_Get)
// {
//     run(StreamType::Color_Stream, "Saturation");
// }

// TEST_F(ControlsSetGetTest, Color_Sharpness_Set_Get)
// {
//     run(StreamType::Color_Stream, "Sharpness");
// }

// TEST_F(ControlsSetGetTest, Color_WhiteBalance_Set_Get)
// {
//     run(StreamType::Color_Stream, "WhiteBalance");
// }