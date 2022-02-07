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

vector<long> tsMeasurements;
void collectPNPMeasurements(Sample s)
{
    tsMeasurements.push_back(TimeUtils::getCurrentTimestamp());
    cpuSamples.push_back(s.Cpu_per);
    memSamples.push_back(s.Mem_MB - memoryBaseLine);
}

class StabilityTest : public TestBase
{
public:
    int _iterations;
    bool _isRandom;
    bool _isContent;
    void configure(int StreamDuration, int Iterations, bool isRandom, bool isContent = false)
    {
        Logger::getLogger().log("Configuring stream duration to: " + to_string(StreamDuration), "Test", LOG_INFO);
        testDuration = StreamDuration;
        Logger::getLogger().log("Configuring test iterations to: " + to_string(Iterations), "Test", LOG_INFO);
        _iterations = Iterations;
        switch (isRandom)
        {
        case 1:
            Logger::getLogger().log("Configuring stability test type to: Random", "Test", LOG_INFO);
            break;

        case 0:
            Logger::getLogger().log("Configuring stability test type to: Normal", "Test", LOG_INFO);
            break;
        }
        _isRandom = isRandom;
        if (isContent)
        {
            Logger::getLogger().log("Content test enabled", "Test", LOG_INFO);
            fa.reset();
            fa.configure(FileUtils::join(testBasePath, name), 10, 0);

        }
        _isContent = isContent;
        isContentTest = isContent;
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

    void runWithPNP(vector<vector<StreamType>> streams)
    {
        SystemMonitor sysMon;
        Logger::getLogger().log("Starting PNP measurements", "Test");
        cpuSamples.clear();
        memSamples.clear();
        tsMeasurements.clear();
        sysMon.StartMeasurment(collectPNPMeasurements, 1000);

        run(streams);

        string testBasePath = FileUtils::join("/home/nvidia/Logs", sid);
        name = ::testing::UnitTest::GetInstance()->current_test_info()->name();

        // Creating test folder
        string testPath = FileUtils::join(testBasePath, name);
        sysMon.StopMeasurment();
        ofstream pnpDataCsv;
        string pnpDataPath = FileUtils::join(testPath, "pnp_data.csv");
        pnpDataCsv.open(pnpDataPath, std::ios_base::app);
        pnpDataCsv << "Timestamp,Memory consumption(MB),CPU consumption(%)\n";
        for (int i = 0; i < memSamples.size(); i++)
        {
            pnpDataCsv << tsMeasurements[i] << "," << memSamples[i] << "," << cpuSamples[i] << "\n";
        }
    }

    void run(vector<vector<StreamType>> streams)
    {
        string name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        vector<Profile> StreamCollection;
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        Logger::getLogger().log("               " + name + " Stability Test ", "Test", LOG_INFO);
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        string failedIterations = "Test Failed in Iterations: ";

        SequentialFrameDropsMetric met_seq;
        FramesArrivedMetric met_arrived;
        FirstFrameDelayMetric met_delay;
        FpsValidityMetric met_fps;
        FrameSizeMetric met_frame_size;
        FrameDropIntervalMetric met_drop_interval;
        FrameDropsPercentageMetric met_drop_percent;
        IDCorrectnessMetric met_id_cor;

        CorruptedMetric met_corrupted;
        FreezeMetric met_freeze;
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

        if (_isContent)
        {
            contentMetrics.push_back(&met_corrupted);
            contentMetrics.push_back(&met_freeze);
        }

        Sensor depthSensor = cam.GetDepthSensor();
        Sensor irSensor = cam.GetIRSensor();
        Sensor colorSensor = cam.GetColorSensor();

        if (_isContent)
        {
            depthSensor.copyFrameData = true;
            irSensor.copyFrameData = true;
            colorSensor.copyFrameData = true;
        }

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

            if (_isContent)
            {
                fa.start_collection();
            }

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
                    irSensor.Configure(StreamCollection[i]);
                }
                else if (StreamCollection[i].streamType == StreamType::Color_Stream)
                {
                    Logger::getLogger().log("Color Profile Used: " + StreamCollection[i].GetText(), "Test");
                    colorSensor.Configure(StreamCollection[i]);
                }
            }
            long startTime = TimeUtils::getCurrentTimestamp();
            //int slept=0;
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
               // std::this_thread::sleep_for(std::chrono::seconds(1));
                //slept+=1;
            }
            if (IRUsed)
            {
                ir_collectFrames = true;
                irSensor.Start(AddFrame);
            }

            std::this_thread::sleep_for(std::chrono::seconds(testDuration));
            if (_isContent)
            {
                fa.stop_collection();
                fa.save_results();
            }

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
            met_arrived.setParams(MetricDefaultTolerances::get_tolerance_FramesArrived(), startTime, testDuration);
            met_delay.setParams(MetricDefaultTolerances::get_tolerance_FirstFrameDelay(), startTime);
            met_fps.setParams(MetricDefaultTolerances::get_tolerance_FpsValidity());
            met_frame_size.setParams(MetricDefaultTolerances::get_tolerance_FrameSize());
            met_drop_interval.setParams(MetricDefaultTolerances::get_tolerance_FrameDropInterval());
            met_drop_percent.setParams(MetricDefaultTolerances::get_tolerance_FrameDropsPercentage());
            met_id_cor.setParams(MetricDefaultTolerances::get_tolerance_IDCorrectness());
            // met_md_cor.setParams(1);

            if (_isContent)
            {
                met_corrupted.setParams(MetricDefaultTolerances::get_tolerance_corrupted());
                met_freeze.setParams(MetricDefaultTolerances::get_tolerance_freeze());
            }

            bool result = CalcMetrics(i);
            if (!result)
            {
                testStatus = false;
                failedIterations += to_string(i) + ", ";
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
        ASSERT_TRUE(testStatus);
    }
};
TEST_F(StabilityTest, Normal)
{
    vector<vector<StreamType>> streams;
    vector<StreamType> sT;
    sT.push_back(StreamType::Depth_Stream);
    sT.push_back(StreamType::IR_Stream);
    sT.push_back(StreamType::Color_Stream);

    //vector<StreamType> sT2;
    //sT2.push_back(StreamType::IR_Stream);
    streams.push_back(sT);
    //streams.push_back(sT2);
    configure(30, 1500, false);
    run(streams);
}

TEST_F(StabilityTest, Random)
{
    vector<vector<StreamType>> streams;
    vector<StreamType> sT;
    sT.push_back(StreamType::Depth_Stream);
    vector<StreamType> sT2;
    sT2.push_back(StreamType::Color_Stream);
    vector<StreamType> sT3;
    sT3.push_back(StreamType::IR_Stream);
    vector<StreamType> sT4;
    sT4.push_back(StreamType::Depth_Stream);
    sT4.push_back(StreamType::IR_Stream);
    sT4.push_back(StreamType::Color_Stream);
    vector<StreamType> sT5;
    sT5.push_back(StreamType::Depth_Stream);
    sT5.push_back(StreamType::IR_Stream);
    //sT5.push_back(StreamType::Color_Stream);
    vector<StreamType> sT6;
    sT6.push_back(StreamType::Depth_Stream);
    //sT6.push_back(StreamType::IR_Stream);
    sT6.push_back(StreamType::Color_Stream);

    vector<StreamType> sT7;
    //sT7.push_back(StreamType::Depth_Stream);
    sT7.push_back(StreamType::IR_Stream);
    sT7.push_back(StreamType::Color_Stream);

    streams.push_back(sT);
    streams.push_back(sT2);
    streams.push_back(sT3);
    streams.push_back(sT4);
    streams.push_back(sT5);
    streams.push_back(sT6);
    streams.push_back(sT7);

    configure(30, 1500, true);
    run(streams);
}

TEST_F(StabilityTest, ContentRandom)
{
    IgnoreMetricAllStreams("First frame delay");
    IgnoreMetricAllStreams("Sequential frame drops");
    IgnoreMetricAllStreams("Frame drops interval");
    IgnoreMetricAllStreams("Frame drops percentage");
    IgnoreMetricAllStreams("Frames arrived");
    IgnoreMetricAllStreams("FPS Validity");
    IgnoreMetricAllStreams("Frame Size");
    IgnoreMetricAllStreams("ID Correctness");

    vector<vector<StreamType>> streams;
    vector<StreamType> sT;
    sT.push_back(StreamType::Depth_Stream);
    vector<StreamType> sT2;
    sT2.push_back(StreamType::Color_Stream);
    vector<StreamType> sT3;
    sT3.push_back(StreamType::IR_Stream);
    vector<StreamType> sT4;
    sT4.push_back(StreamType::Depth_Stream);
    sT4.push_back(StreamType::Color_Stream);
    vector<StreamType> sT5;
    sT5.push_back(StreamType::Depth_Stream);
    sT5.push_back(StreamType::IR_Stream);
    sT5.push_back(StreamType::Color_Stream);


    streams.push_back(sT);
    streams.push_back(sT2);
    streams.push_back(sT3);
    streams.push_back(sT4);
    // streams.push_back(sT5);

    configure(10, 100, true, true);
    run(streams);
}

TEST_F(StabilityTest, PnpRandom)
{
    vector<vector<StreamType>> streams;
    vector<StreamType> sT;
    sT.push_back(StreamType::Depth_Stream);
    vector<StreamType> sT2;
    sT2.push_back(StreamType::Color_Stream);
    vector<StreamType> sT3;
    sT3.push_back(StreamType::IR_Stream);
    vector<StreamType> sT4;
    sT4.push_back(StreamType::Depth_Stream);
    sT4.push_back(StreamType::IR_Stream);
    sT4.push_back(StreamType::Color_Stream);
    vector<StreamType> sT5;
    sT5.push_back(StreamType::Depth_Stream);
    sT5.push_back(StreamType::IR_Stream);
    //sT5.push_back(StreamType::Color_Stream);
    vector<StreamType> sT6;
    sT6.push_back(StreamType::Depth_Stream);
    //sT6.push_back(StreamType::IR_Stream);
    sT6.push_back(StreamType::Color_Stream);

    vector<StreamType> sT7;
    //sT7.push_back(StreamType::Depth_Stream);
    sT7.push_back(StreamType::IR_Stream);
    sT7.push_back(StreamType::Color_Stream);

    streams.push_back(sT);
    streams.push_back(sT2);
    streams.push_back(sT3);
    streams.push_back(sT4);
    streams.push_back(sT5);
    streams.push_back(sT6);
    streams.push_back(sT7);
    configure(30, 1500, true);
    runWithPNP(streams);
}

