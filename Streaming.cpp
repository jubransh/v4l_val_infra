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

class StreamingTest : public TestBase
{
public:
    void configure(int StreamDuration)
    {
        Logger::getLogger().log("Configuring stream duration to: " + to_string(StreamDuration), "Test", LOG_INFO);
        testDuration = StreamDuration;
    }
    void run(vector<StreamType> streams)
    {
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
        Sensor irSensor = cam.GetIRSensor();
        Sensor colorSensor = cam.GetColorSensor();

        vector<vector<Profile>> profiles = GetProfiles(streams);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        Logger::getLogger().log("               Streaming Test ", "Test", LOG_INFO);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);

        vector<Profile> pR;
        bool DepthUsed=false;
        bool ColorUsed=false;
        bool IRUsed=false;
        for (int f=0;f< streams.size();f++)
        {
            if (streams[f]==StreamType::Depth_Stream)
                DepthUsed=true;
            else if (streams[f]==StreamType::IR_Stream)
                IRUsed=true;
            else if (streams[f]==StreamType::Color_Stream)
                ColorUsed=true;
        }
        for (int j = 0; j < profiles.size(); j++)
        {
            Logger::getLogger().log("Started Iteration: " + to_string(j), "Test");
            initFrameLists();

            pR.clear();
            for (int i =0;i<profiles[j].size();i++)
            {
                if (profiles[j][i].streamType==StreamType::Depth_Stream)
                {
                    Logger::getLogger().log("Depth Profile Used: " + profiles[j][i].GetText(), "Test");
                    depthSensor.Configure(profiles[j][i]);
                    pR.push_back(profiles[j][i]);

                }
                else if (profiles[j][i].streamType==StreamType::IR_Stream)
                {
                    Logger::getLogger().log("IR Profile Used: " + profiles[j][i].GetText(), "Test");
                    irSensor.Configure(profiles[j][i]);
                    pR.push_back(profiles[j][i]);
                }
                else if (profiles[j][i].streamType==StreamType::Color_Stream)
                {
                    Logger::getLogger().log("Color Profile Used: " + profiles[j][i].GetText(), "Test");
                    colorSensor.Configure(profiles[j][i]);
                    pR.push_back(profiles[j][i]);
                }
            }                      
            setCurrentProfiles(pR);

            long startTime = TimeUtils::getCurrentTimestamp();
            //int slept = 0;
            // collectFrames=true;
            if (ColorUsed)
            {   
                color_collectFrames= true;
                colorSensor.Start(AddFrame);
                //std::this_thread::sleep_for(std::chrono::seconds(1));
                //slept+=1;
            }
            if (DepthUsed)
            {
                depth_collectFrames = true;
                depthSensor.Start(AddFrame);
                //std::this_thread::sleep_for(std::chrono::seconds(1));
                //slept+=1;
            }
            if (IRUsed)
            {
                ir_collectFrames = true;
                irSensor.Start(AddFrame);
            }


            long startTime2 = TimeUtils::getCurrentTimestamp();
            std::this_thread::sleep_for(std::chrono::seconds(testDuration-slept));

            // collectFrames=false;
            if (ColorUsed)
            {
                color_collectFrames= false;
                colorSensor.Stop();
                colorSensor.Close();
                //std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (DepthUsed)
            {
                depth_collectFrames = false;
                depthSensor.Stop();
                depthSensor.Close();
                //std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (IRUsed)
            {
                ir_collectFrames = false;
                irSensor.Stop();
                irSensor.Close();
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
            bool result = CalcMetrics(j);
            if (!result)
            {
                testStatus = false;
                failedIterations+= to_string(j)+", ";
            }

            //Logger::getLogger().log("Iteration :" + to_string(j) + " Done - Iteration Result: " + to_string(result), "Run");
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

TEST_F(StreamingTest, DepthStreamingTest)
{
    configure(30);
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);
    run(streams);
}

TEST_F(StreamingTest, IRStreamingTest)
{
    configure(30);
    vector<StreamType> streams;
    streams.push_back(StreamType::IR_Stream);
    run(streams);
}
TEST_F(StreamingTest, ColorStreamingTest)
{
    configure(30);
    vector<StreamType> streams;
    streams.push_back(StreamType::Color_Stream);
    run(streams);
}
TEST_F(StreamingTest, DepthIRStreamingTest)
{
    configure(30);
    vector<StreamType> streams;
    streams.push_back(StreamType::IR_Stream);
    streams.push_back(StreamType::Depth_Stream);
    run(streams);
}
TEST_F(StreamingTest, DepthColorStreamingTest)
{
    configure(30);
    vector<StreamType> streams;
    streams.push_back(StreamType::Color_Stream);
    streams.push_back(StreamType::Depth_Stream);
    run(streams);
}
TEST_F(StreamingTest, IRColorStreamingTest)
{
    configure(10);
    vector<StreamType> streams;
    streams.push_back(StreamType::Color_Stream);
    streams.push_back(StreamType::IR_Stream);
    run(streams);
}
TEST_F(StreamingTest, DepthIRColorStreamingTest)
{
    configure(30);
    vector<StreamType> streams;
    streams.push_back(StreamType::Color_Stream);
    streams.push_back(StreamType::Depth_Stream);
    streams.push_back(StreamType::IR_Stream);
    run(streams);
}



