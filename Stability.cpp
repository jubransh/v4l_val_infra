#include <ctime>
#include <sys/mman.h>
#include "MetaData.h"

using namespace std;
#include <gtest/gtest.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <errno.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <array>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <thread>
#include "infra/TestInfra.cpp"

class StabilityTest : public TestBase
{
public:
    int _iterations;
    bool _isRandom;
    void configure(int StreamDuration, int Iterations, bool isRandom)
    {
        testDuration = StreamDuration;
        _iterations = Iterations;
        _isRandom = isRandom;
    }
    vector<Profile> getRandomProfile(vector<vector<StreamType>> streams)
    {
        vector<vector<Profile>> allProfilesComb;
        for (int i = 0; i < streams.size(); i++)
        {
            auto profilesComb = GetProfiles(streams[i]);
            for (int j = 0; j < profilesComb.size(); j++)
            {
                allProfilesComb.push_back(profilesComb[j]);
            }
        }

        /* initialize random seed: */
        srand(time(NULL));
        int randNum = (std::rand() % (allProfilesComb.size()));
        return allProfilesComb[randNum];
    }

    void run(vector<vector<StreamType>> streams)
    {
        string name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        vector<Profile> StreamCollection;
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        Logger::getLogger().log("               " + name +" Stability Test ", "Test", LOG_INFO);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        bool testStatus = true;
        string failedIterations = "Test Failed in Iterations: ";

        SequentialFrameDropsMetric met_seq;
        FramesArrivedMetric met_arrived;
        FirstFrameDelayMetric met_delay;
        FpsValidityMetric met_fps;
        FrameSizeMetric met_frame_size;
        FrameDropIntervalMetric met_drop_interval;
        FrameDropsPercentageMetric met_drop_percent;
        IDCorrectnessMetric met_id_cor;
        // MetaDataCorrectnessMetric met_md_cor;

        metrics.push_back(&met_seq);
        metrics.push_back(&met_arrived);
        metrics.push_back(&met_drop_interval);
        metrics.push_back(&met_drop_percent);
        metrics.push_back(&met_delay);
        metrics.push_back(&met_fps);
        metrics.push_back(&met_frame_size);
        metrics.push_back(&met_id_cor);
        // metrics.push_back(&met_md_cor);

        Sensor depthSensor = cam.GetDepthSensor();
        Sensor colorSensor = cam.GetColorSensor();
        bool DepthUsed;
        bool ColorUsed;
        bool IRUsed;

        for (int i = 0; i < _iterations; i++)
        {
            DepthUsed = false;
            ColorUsed = false;
            IRUsed = false;
            if (_isRandom)
                StreamCollection = getRandomProfile(streams);
            else
                StreamCollection = GetHighestCombination(streams[0]);

            for (int f = 0; f < StreamCollection.size(); f++)
            {
                if (StreamCollection[f].streamType == StreamType::Depth_Stream)
                    DepthUsed = true;
                else if (StreamCollection[f].streamType == StreamType::IR_Stream)
                    IRUsed = true;
                else if (StreamCollection[f].streamType == StreamType::Color_Stream)
                    ColorUsed = true;
            }
            Logger::getLogger().log("Started Iteration: " + to_string(i), "Test");
            initFrameLists();
            setCurrentProfiles(StreamCollection);

            for (int i = 0; i < StreamCollection.size(); i++)
            {
                if (StreamCollection[i].streamType == StreamType::Depth_Stream)
                {
                    Logger::getLogger().log("Depth Profile Used: " + StreamCollection[i].GetText(), "Test");
                    depthSensor.Configure(StreamCollection[i]);
                }
                else if (StreamCollection[i].streamType == StreamType::IR_Stream)
                {
                    Logger::getLogger().log("IR Profile Used: " + StreamCollection[i].GetText(), "Test");
                    depthSensor.Configure(StreamCollection[i]);
                }
                else if (StreamCollection[i].streamType == StreamType::Color_Stream)
                {
                    Logger::getLogger().log("Color Profile Used: " + StreamCollection[i].GetText(), "Test");
                    colorSensor.Configure(StreamCollection[i]);
                }
            }
            long startTime = TimeUtils::getCurrentTimestamp();
            collectFrames = true;
            if (DepthUsed || IRUsed)
            {
                depthSensor.Start(AddFrame);
            }
            if (ColorUsed)
            {
                colorSensor.Start(AddFrame);
            }

            std::this_thread::sleep_for(std::chrono::seconds(testDuration));
            collectFrames = false;
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
            
            met_seq.setParams(MetricDefaultTolerances::get_tolerance_SequentialFrameDrops());
            met_arrived.setParams(MetricDefaultTolerances::get_tolerance_FramesArrived(),startTime, testDuration);
            met_delay.setParams(MetricDefaultTolerances::get_tolerance_FirstFrameDelay(),startTime);
            met_fps.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity());
            met_frame_size.setParams(MetricDefaultTolerances::get_tolerance_FrameSize());
            met_drop_interval.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval());
            met_drop_percent.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage());
            met_id_cor.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness());
            // met_md_cor.setParams(1);
            bool result = CalcMetrics(i);
            if (!result)
            {
                testStatus = false;
                failedIterations+= to_string(i)+", ";
            }
        }

        Logger::getLogger().log("Test Summary:", "Run");
        Logger::getLogger().log( testStatus?"Pass":"Fail", "Run");
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
TEST_F(StabilityTest, Normal)
{
    vector<vector<StreamType>> streams;
    vector<StreamType> sT;
    sT.push_back(StreamType::Depth_Stream);
    sT.push_back(StreamType::Color_Stream);
    vector<StreamType> sT2;
    sT2.push_back(StreamType::IR_Stream);
    streams.push_back(sT);
    streams.push_back(sT2);
    configure(5, 200, false);
    run(streams);
}

TEST_F(StabilityTest, Random)
{
    vector<vector<StreamType>> streams;
    vector<StreamType> sT;
    sT.push_back(StreamType::Depth_Stream);
    vector<StreamType> sT3;
    sT3.push_back(StreamType::Color_Stream);
    vector<StreamType> sT2;
    sT2.push_back(StreamType::IR_Stream);
    vector<StreamType> sT4;
    sT4.push_back(StreamType::Depth_Stream);
    sT4.push_back(StreamType::Color_Stream);
    streams.push_back(sT);
    streams.push_back(sT2);
    streams.push_back(sT3);
    streams.push_back(sT4);
    configure(15, 250, true);
    run(streams);
}

TEST_F(StabilityTest, Test)
{
    // configure(5);
    vector<vector<StreamType>> streams;
    vector<StreamType> sT;
    sT.push_back(StreamType::Depth_Stream);
    sT.push_back(StreamType::Color_Stream);
    streams.push_back(sT);
    vector<Profile> stream = getRandomProfile(streams);
     SequentialFrameDropsMetric met_seq;
        FramesArrivedMetric met_arrived;
        FirstFrameDelayMetric met_delay;
        FpsValidityMetric met_fps;
        FrameSizeMetric met_frame_size;
        FrameDropIntervalMetric met_drop_interval;
        FrameDropsPercentageMetric met_drop_percent;
        IDCorrectnessMetric met_id_cor;
        // MetaDataCorrectnessMetric met_md_cor;

        metrics.push_back(&met_seq);
        metrics.push_back(&met_arrived);
        metrics.push_back(&met_drop_interval);
        metrics.push_back(&met_drop_percent);
        metrics.push_back(&met_delay);
        metrics.push_back(&met_fps);
        metrics.push_back(&met_frame_size);
        metrics.push_back(&met_id_cor);
        // metrics.push_back(&met_md_cor);

    Sensor depthSensor = cam.GetDepthSensor();
    Sensor colorSensor = cam.GetColorSensor();
    for(int i=0;i<3; i++)
    {
        initFrameLists();
        setCurrentProfiles(stream);
        depthSensor.Configure(stream[0]);
        colorSensor.Configure(stream[1]);
        long startTime = TimeUtils::getCurrentTimestamp();
        depthSensor.Start(AddFrame);
        colorSensor.Start(AddFrame);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        depthSensor.Stop();
        depthSensor.Close();
        colorSensor.Stop();
        colorSensor.Close();

                    met_seq.setParams(MetricDefaultTolerances::get_tolerance_SequentialFrameDrops());
            met_arrived.setParams(MetricDefaultTolerances::get_tolerance_FramesArrived(),startTime, 3);
            met_delay.setParams(MetricDefaultTolerances::get_tolerance_FirstFrameDelay(),startTime);
            met_fps.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity());
            met_frame_size.setParams(MetricDefaultTolerances::get_tolerance_FrameSize());
            met_drop_interval.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval());
            met_drop_percent.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage());
            met_id_cor.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness());
            // met_md_cor.setParams(1);
            bool result = CalcMetrics(i);
            // if (!result)
            // {
            //     testStatus = false;
            //     failedIterations+= to_string(i)+", ";
            // }
    }
    


}
