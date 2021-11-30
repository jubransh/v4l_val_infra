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
    LongTest()
    {
        isPNPtest = true;
    }
    void configure(int StreamDuration)
    {
        testDuration = StreamDuration;
    }
    void run(vector<StreamType> streams)
    {
        CalculateMemoryBaseLine();
        int iterationDuration = 60;
        bool testStatus = true;
        string failedIterations = "Test Failed in Iterations: ";

        SequentialFrameDropsMetric met_seq;
        FpsValidityMetric met_fps;
        FrameDropIntervalMetric met_drop_interval;
        FrameDropsPercentageMetric met_drop_percent;

        CPUMetric met_cpu;
        MemMetric met_mem;
        AsicTempMetric met_asic_temp;
        ProjTempMetric met_projector_temp;


        metrics.push_back(&met_seq);
        metrics.push_back(&met_drop_interval);
        metrics.push_back(&met_drop_percent);
        metrics.push_back(&met_fps);
        pnpMetrics.push_back(&met_cpu);
        pnpMetrics.push_back(&met_mem);
        pnpMetrics.push_back(&met_asic_temp);
        pnpMetrics.push_back(&met_projector_temp);
        

        Sensor depthSensor = cam.GetDepthSensor();
        Sensor colorSensor = cam.GetColorSensor();

        vector<Profile> profiles = GetHighestCombination(streams);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        Logger::getLogger().log("               Long Test ", "Test", LOG_INFO);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);

        vector<Profile> pR;
        bool DepthUsed = false;
        bool ColorUsed = false;
        bool IRUsed = false;
        for (int f = 0; f < streams.size(); f++)
        {
            if (streams[f] == StreamType::Depth_Stream)
                DepthUsed = true;
            else if (streams[f] == StreamType::IR_Stream)
                IRUsed = true;
            else if (streams[f] == StreamType::Color_Stream)
                ColorUsed = true;
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
                depthSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
            else if (profiles[j].streamType == StreamType::Color_Stream)
            {
                Logger::getLogger().log("Color Profile Used: " + profiles[j].GetText(), "Test");
                colorSensor.Configure(profiles[j]);
                pR.push_back(profiles[j]);
            }
        }
        setCurrentProfiles(pR);

        if (DepthUsed || IRUsed)
        {
            depthSensor.Start(AddFrame);
        }
        if (ColorUsed)
        {
            colorSensor.Start(AddFrame);
        }
        
        int Iterations = testDuration / iterationDuration;
        for (int j = 0; j < Iterations; j++)
        {
            Logger::getLogger().log("Started Iteration: " + to_string(j), "Test");
            initFrameLists();
            initSamplesList();
            SystemMonitor sysMon2;
            Logger::getLogger().log("Starting PNP measurements","Test");
            sysMon2.StartMeasurment(addToPnpList,250);
            collectFrames = true;
            Logger::getLogger().log("collecting frames for half iteration duration("+to_string(iterationDuration/2)+"Seconds)","Test");
            std::this_thread::sleep_for(std::chrono::seconds(iterationDuration/2));

            collectFrames = false;
            Logger::getLogger().log("Stopping PNP measurements","Test");
            sysMon2.StopMeasurment();
            Logger::getLogger().log("Getting Asic temperature","Test");
            asicSamples.push_back(cam.GetAsicTemperature());
            Logger::getLogger().log("Getting Projector temperature","Test");
            projectorSamples.push_back(cam.GetProjectorTemperature());
            long startCalcTime = TimeUtils::getCurrentTimestamp();

            met_seq.setParams(MetricDefaultTolerances::get_tolerance_SequentialFrameDrops());
            met_fps.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity());
            met_drop_interval.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval());
            met_drop_percent.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage());

            met_cpu.setParams(MetricDefaultTolerances::get_tolerance_CPU());
            met_mem.setParams(MetricDefaultTolerances::get_tolerance_Memory());
            met_asic_temp.setParams(MetricDefaultTolerances::get_tolerance_asic_temperature());
            met_projector_temp.setParams(MetricDefaultTolerances::get_tolerance_projector_temperature());
            bool result = CalcMetrics(j);
            if (!result)
            {
                testStatus = false;
                failedIterations += to_string(j) + ", ";
            }
            long endCalcTime = TimeUtils::getCurrentTimestamp();
            long calcDuration = (endCalcTime - startCalcTime)/1000;
            Logger::getLogger().log("Calculations took: "+to_string(calcDuration)+"Seconds","Test");
            Logger::getLogger().log("Going to sleep for the rest of the iteration duration("+to_string(iterationDuration)+"Seconds), for: "+to_string(iterationDuration/2 - calcDuration)+"Seconds","Test");
            std::this_thread::sleep_for(std::chrono::seconds(iterationDuration/2 - calcDuration));
        }
        if (DepthUsed || IRUsed)
        {
            depthSensor.Stop();
            depthSensor.Close();
        }
        if (ColorUsed)
        {
            colorSensor.Stop();
            colorSensor.Close();
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
    configure(3*60);
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);
    streams.push_back(StreamType::Color_Stream);
    // IgnorePNPMetric("CPU Consumption");
    run(streams);
}
