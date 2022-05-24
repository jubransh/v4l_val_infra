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

class LongTest : public TestBase
{
public:
    bool _captureTempWhileStream;
    bool _isContent;
    string _profileText = "";

    LongTest()
    {
        isPNPtest = true;
    }
    void set_profile(string profile)
    {
        _profileText = profile;
    }
    void configure(int StreamDuration, bool captureTempWhileStream, bool isContent = false)
    {
        Logger::getLogger().log("Configuring stream duration to: " + to_string(StreamDuration), "Test", LOG_INFO);
        testDuration = StreamDuration;
        _captureTempWhileStream = captureTempWhileStream;

        if (isContent)
        {
            Logger::getLogger().log("Content test enabled", "Test", LOG_INFO);
            fa.reset();
            fa.configure(FileUtils::join(testBasePath, name), 10, 16);
        }
        _isContent = isContent;
        isContentTest = isContent;
    }
    void run(vector<StreamType> streams, int fps = 0)
    {
        CalculateMemoryBaseLine();
        int iterationDuration = 60;
        string failedIterations = "Test Failed in Iterations: ";

        SequentialFrameDropsMetric met_seq;
        FpsValidityMetric met_fps;
        FrameDropIntervalMetric met_drop_interval;
        FrameDropsPercentageMetric met_drop_percent;
        MetaDataCorrectnessMetric met_md_cor;

        CPUMetric met_cpu;
        MemMetric met_mem;
        AsicTempMetric met_asic_temp;
        ProjTempMetric met_projector_temp;

        CorruptedMetric met_corrupted;
        FreezeMetric met_freeze;

        metrics.push_back(&met_seq);
        metrics.push_back(&met_drop_interval);
        metrics.push_back(&met_drop_percent);
        metrics.push_back(&met_fps);
        metrics.push_back(&met_md_cor);
        pnpMetrics.push_back(&met_cpu);
        pnpMetrics.push_back(&met_mem);
        pnpMetrics.push_back(&met_asic_temp);
        pnpMetrics.push_back(&met_projector_temp);

        if (_isContent)
        {
            contentMetrics.push_back(&met_corrupted);
            contentMetrics.push_back(&met_freeze);
        }

        Sensor depthSensor = cam.GetDepthSensor();
        Sensor irSensor = cam.GetIRSensor();
        Sensor colorSensor = cam.GetColorSensor();
        Sensor imuSensor = cam.GetIMUSensor();

        if (_isContent)
        {
            depthSensor.copyFrameData = true;
            irSensor.copyFrameData = true;
            colorSensor.copyFrameData = true;
            bool res;
            Logger::getLogger().log("Setting Laser Power to 90 for Depth Sensor", "Test");
		    res = depthSensor.SetControl(DS5_CAMERA_CID_MANUAL_LASER_POWER, 90);
		    Logger::getLogger().log("Setting Laser Power to 90 for Depth Sensor: " + (string)(res ? "Passed" : "Failed"), "Test");
            // Logger::getLogger().log("Disabling AutoExposure for Depth Sensor", "Test");
		    // res = depthSensor.SetControl(V4L2_CID_EXPOSURE_AUTO, 1);
		    // Logger::getLogger().log("Disabling AutoExposure for Depth Sensor: " + (string)(res ? "Passed" : "Failed"), "Test");
            // Logger::getLogger().log("Setting Exposure to 50 for Depth Sensor", "Test");
		    // res = depthSensor.SetControl(V4L2_CID_EXPOSURE_ABSOLUTE, 50);
		    // Logger::getLogger().log("Setting Exposure Power to 50 for Depth Sensor: " + (string)(res ? "Passed" : "Failed"), "Test");

        }
        vector<Profile> profiles;
        if (_profileText.compare("") != 0)
        {
            Logger::getLogger().log("Configuring Static Profile to: " + _profileText, "Test", LOG_INFO);
            ProfileGenerator pG;
            profiles = pG.GetProfilesByString(_profileText);
        }
        else if (fps!=0)
            profiles = GetHighestCombination(streams,fps);
        else
            profiles = GetHighestCombination(streams);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        Logger::getLogger().log("               Long Test ", "Test", LOG_INFO);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);

        vector<Profile> pR;
        bool DepthUsed = false;
        bool ColorUsed = false;
        bool IRUsed = false;
        bool ImuUsed = false;
        for (int f = 0; f < streams.size(); f++)
        {
            if (streams[f] == StreamType::Depth_Stream)
                DepthUsed = true;
            else if (streams[f] == StreamType::IR_Stream)
                IRUsed = true;
            else if (streams[f] == StreamType::Color_Stream)
                ColorUsed = true;
            else if (streams[f] == StreamType::Imu_Stream)
                    ImuUsed = true;
        }
        for (int j = 0; j < profiles.size(); j++)
        {
            if (profiles[j].streamType == StreamType::Depth_Stream)
            {
                Logger::getLogger().log("Depth Profile Used: " + profiles[j].GetText(), "Test");
                depthSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
            else if (profiles[j].streamType == StreamType::IR_Stream)
            {
                Logger::getLogger().log("IR Profile Used: " + profiles[j].GetText(), "Test");
                irSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
            else if (profiles[j].streamType == StreamType::Color_Stream)
            {
                Logger::getLogger().log("Color Profile Used: " + profiles[j].GetText(), "Test");
                colorSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
            else if (profiles[j].streamType == StreamType::Imu_Stream)
            {
                Logger::getLogger().log("IMU Profile Used: " + profiles[j].GetText(), "Test");
                imuSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
        }
        setCurrentProfiles(pR);
        if (ColorUsed)
        {
            colorSensor.Start(AddFrame);
            //std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (DepthUsed)
        {
            depthSensor.Start(AddFrame);
            //std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (IRUsed)
        {
            irSensor.Start(AddFrame);
        }
        if (ImuUsed)
        {

            imuSensor.Start(AddFrame);
        }

        int Iterations = testDuration / iterationDuration;
        for (int j = 0; j < Iterations; j++)
        {
            if (_isContent)
            {
                fa.start_collection();
            }
            Logger::getLogger().log("Started Iteration: " + to_string(j), "Test");
            initFrameLists();
            initSamplesList();
            SystemMonitor sysMon2;
            Logger::getLogger().log("Starting PNP measurements", "Test");
            sysMon2.StartMeasurment(addToPnpList, 250);
            // collectFrames = true;
            depth_collectFrames = true;
            ir_collectFrames = true;
            color_collectFrames = true;
            imu_collectFrames = true;
            Logger::getLogger().log("collecting frames for half iteration duration(" + to_string(iterationDuration / 2) + "Seconds)", "Test");
            int duration = iterationDuration / 2;
            if (!_captureTempWhileStream)
                std::this_thread::sleep_for(std::chrono::seconds(duration));
            else
            {
                for (int d = 0; d < duration; d++)
                {
                    Logger::getLogger().log("Getting Asic temperature", "Test");
                    asicSamples.push_back(cam.GetAsicTemperature());
                    Logger::getLogger().log("Getting Projector temperature", "Test");
                    projectorSamples.push_back(cam.GetProjectorTemperature());
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            if (_isContent)
            {
                fa.stop_collection();
                fa.save_results();
            }
            depth_collectFrames = false;
            ir_collectFrames = false;
            color_collectFrames = false;
            imu_collectFrames = false;
            
            Logger::getLogger().log("Stopping PNP measurements", "Test");
            sysMon2.StopMeasurment();
            if (!_captureTempWhileStream)
            {
                Logger::getLogger().log("Getting Asic temperature", "Test");
                asicSamples.push_back(cam.GetAsicTemperature());
                Logger::getLogger().log("Getting Projector temperature", "Test");
                projectorSamples.push_back(cam.GetProjectorTemperature());
            }

            long startCalcTime = TimeUtils::getCurrentTimestamp();

            met_seq.setParams(MetricDefaultTolerances::get_tolerance_SequentialFrameDrops());
            met_fps.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity());
            met_drop_interval.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval());
            met_drop_percent.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage());
            met_md_cor.setParams(1);

            met_cpu.setParams(MetricDefaultTolerances::get_tolerance_CPU());
            met_mem.setParams(MetricDefaultTolerances::get_tolerance_Memory());
            met_asic_temp.setParams(MetricDefaultTolerances::get_tolerance_asic_temperature());
            met_projector_temp.setParams(MetricDefaultTolerances::get_tolerance_projector_temperature());

            if (_isContent)
            {
                met_corrupted.setParams(MetricDefaultTolerances::get_tolerance_corrupted());
                met_freeze.setParams(MetricDefaultTolerances::get_tolerance_freeze());
            }

            bool result = CalcMetrics(j);
            if (!result)
            {
                testStatus = false;
                failedIterations += to_string(j) + ", ";
            }
            long endCalcTime = TimeUtils::getCurrentTimestamp();
            long calcDuration = (endCalcTime - startCalcTime) / 1000;
            Logger::getLogger().log("Calculations took: " + to_string(calcDuration) + "Seconds", "Test");
            Logger::getLogger().log("Going to sleep for the rest of the iteration duration(" + to_string(iterationDuration) + "Seconds), for: " + to_string(iterationDuration / 2 - calcDuration) + "Seconds", "Test");
            std::this_thread::sleep_for(std::chrono::seconds(iterationDuration / 2 - calcDuration));
        }

        if (ColorUsed)
        {
            colorSensor.Stop();
            colorSensor.Close();
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
        if (ImuUsed)
        {
            imuSensor.Stop();
            imuSensor.Close();
        }

        Logger::getLogger().log("Test Summary:", "Run");
        Logger::getLogger().log(testStatus ? "Pass" : "Fail", "Run");
        if (!testStatus)
        {
            Logger::getLogger().log(failedIterations, "Run");
        }
        ASSERT_TRUE(testStatus);
    }
};

TEST_F(LongTest, LongStreamTest)
{
    configure(5 *60 * 60, false);
    set_profile("z16_1280x720_30+y8_1280x720_30+yuyv_1280x720_30+imu_0x0_400");
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);
    streams.push_back(StreamType::IR_Stream);
    streams.push_back(StreamType::Color_Stream);
    streams.push_back(StreamType::Imu_Stream);
    // IgnorePNPMetric("CPU Consumption");
    run(streams);
}
TEST_F(LongTest, LongStreamTest_60FPS)
{
    configure(5 * 60 * 60, false);
    set_profile("z16_640x480_60+y8_640x480_60+yuyv_640x480_60+imu_0x0_400");
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);
    streams.push_back(StreamType::IR_Stream);
    streams.push_back(StreamType::Color_Stream);
    streams.push_back(StreamType::Imu_Stream);
    // IgnorePNPMetric("CPU Consumption");
    run(streams,60);
}
/*
TEST_F(LongTest, LongStreamTest_Specific_Profile)
{
    configure(10 * 60 * 60, false);
    set_profile("z16_640x480_5+y8_640x480_5+yuyv_640x480_5");
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);
    streams.push_back(StreamType::IR_Stream);
    streams.push_back(StreamType::Color_Stream);
    // IgnorePNPMetric("CPU Consumption");
    run(streams, 60);
}

TEST_F(LongTest, ContentLongStreamTest)
{
    IgnoreMetricAllStreams("First frame delay");
    IgnoreMetricAllStreams("Sequential frame drops");
    IgnoreMetricAllStreams("Frame drops interval");
    IgnoreMetricAllStreams("Frame drops percentage");
    IgnoreMetricAllStreams("Frames arrived");
    IgnoreMetricAllStreams("FPS Validity");
    IgnoreMetricAllStreams("Frame Size");
    IgnoreMetricAllStreams("ID Correctness");

    IgnorePNPMetric("CPU Consumption");
    IgnorePNPMetric("Memory Consumption");

    configure(5 * 60 * 60, false, true);
    set_profile("z16_1280x720_30+y8_1280x720_30+yuyv_1280x720_30+imu_0x0_400");
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);
    streams.push_back(StreamType::IR_Stream);
    streams.push_back(StreamType::Color_Stream);
    streams.push_back(StreamType::Imu_Stream);
    run(streams);
}

TEST_F(LongTest, TempCaptureLongStreamTest)
{
    configure(10 * 60 * 60, true);
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);
    streams.push_back(StreamType::IR_Stream);
    streams.push_back(StreamType::Color_Stream);
    // IgnorePNPMetric("CPU Consumption");
    run(streams);
}
*/