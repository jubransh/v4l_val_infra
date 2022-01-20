using namespace std;

class ContentSpecialTest : public TestBase
{
public:
    int _iterations;
    bool _is_depth_over_color;
    void configure(int StreamDuration, int Iterations, bool is_depth_over_color)
    {
        Logger::getLogger().log("Configuring stream duration to: " + to_string(StreamDuration), "Test", LOG_INFO);
        testDuration = StreamDuration;
        Logger::getLogger().log("Configuring test iterations to: " + to_string(Iterations), "Test", LOG_INFO);
        _iterations = Iterations;

        Logger::getLogger().log("Content test enabled", "Test", LOG_INFO);
        fa.reset();
        fa.configure(FileUtils::join(testBasePath, name), 10, 0);

        isContentTest = true;
        _is_depth_over_color = is_depth_over_color;
    }

    void run(Profile depth_profile, Profile color_profile)
    {
        string name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        if (_is_depth_over_color)
        {
            Logger::getLogger().log("               " + name + " Depth Over Color Test", "Test", LOG_INFO);
        }
        else
        {
            Logger::getLogger().log("               " + name + " Color Over Depth Test", "Test", LOG_INFO);
        }

        Logger::getLogger().log("=================================================", "Test", LOG_INFO);
        string failedIterations = "Test Failed in Iterations: ";

        CorruptedMetric met_corrupted;
        FreezeMetric met_freeze;

        contentMetrics.push_back(&met_corrupted);
        contentMetrics.push_back(&met_freeze);

        Sensor depthSensor = cam.GetDepthSensor();
        Sensor colorSensor = cam.GetColorSensor();

        depthSensor.copyFrameData = true;
        colorSensor.copyFrameData = true;

        Logger::getLogger().log("Depth Profile Used: " + depth_profile.GetText(), "Test");
        Logger::getLogger().log("Color Profile Used: " + color_profile.GetText(), "Test");
        vector<Profile> StreamCollection;
        StreamCollection.push_back(depth_profile);
        StreamCollection.push_back(color_profile);
        setCurrentProfiles(StreamCollection);

        if (_is_depth_over_color)
        {
            Logger::getLogger().log("Configuring color sensor", "Test", LOG_INFO);
            colorSensor.Configure(color_profile);
            Logger::getLogger().log("Starting color sensor", "Test", LOG_INFO);
            colorSensor.Start(AddFrame);
        }
        else
        {
            Logger::getLogger().log("Configuring depth sensor", "Test", LOG_INFO);
            depthSensor.Configure(depth_profile);
            Logger::getLogger().log("Starting depth sensor", "Test", LOG_INFO);
            depthSensor.Start(AddFrame);
        }
        Logger::getLogger().log("going to sleep for 1 second between sensors", "Test", LOG_INFO);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (int i = 0; i < _iterations; i++)
        {
            Logger::getLogger().log("Started Iteration: " + to_string(i), "Test");
            initFrameLists();

            fa.start_collection();
            collectFrames = true;

            if (_is_depth_over_color)
            {
                Logger::getLogger().log("Configuring depth sensor", "Test", LOG_INFO);
                depthSensor.Configure(depth_profile);
                Logger::getLogger().log("Starting depth sensor", "Test", LOG_INFO);
                depthSensor.Start(AddFrame);
            }
            else
            {
                Logger::getLogger().log("Configuring color sensor", "Test", LOG_INFO);
                colorSensor.Configure(color_profile);
                Logger::getLogger().log("Starting color sensor", "Test", LOG_INFO);
                colorSensor.Start(AddFrame);
            }

            Logger::getLogger().log("Collecting frames for " + to_string(testDuration) + "seconds", "Test", LOG_INFO);
            std::this_thread::sleep_for(std::chrono::seconds(testDuration));
            collectFrames = false;

            fa.stop_collection();
            fa.save_results();

            if (_is_depth_over_color)
            {
                Logger::getLogger().log("Stopping depth sensor", "Test", LOG_INFO);
                depthSensor.Stop();
                depthSensor.Close();
            }
            else
            {
                Logger::getLogger().log("Stopping color sensor", "Test", LOG_INFO);
                colorSensor.Stop();
                colorSensor.Close();
            }

            met_corrupted.setParams(MetricDefaultTolerances::get_tolerance_corrupted());
            met_freeze.setParams(MetricDefaultTolerances::get_tolerance_freeze());

            bool result = CalcMetrics(i);
            if (!result)
            {
                testStatus = false;
                failedIterations += to_string(i) + ", ";
            }
        }

        if (_is_depth_over_color)
        {
            Logger::getLogger().log("Stopping color sensor", "Test", LOG_INFO);
            colorSensor.Stop();
            colorSensor.Close();
        }
        else
        {
            Logger::getLogger().log("Stopping depth sensor", "Test", LOG_INFO);
            depthSensor.Stop();
            depthSensor.Close();
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

TEST_F(ContentSpecialTest, ContentDepthOverColorTest)
{
    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 60;
    dP.streamType = StreamType::Depth_Stream;

    // Color Configuration
    Resolution cR = {0};
    cR.width = 640;
    cR.height = 480;
    Profile cP;
    cP.pixelFormat = V4L2_PIX_FMT_YUYV;
    cP.resolution = cR;
    cP.fps = 60;
    cP.streamType = StreamType::Color_Stream;

    configure(5, 50, true);
    run(dP, cP);
}

TEST_F(ContentSpecialTest, ContentColorOverDepthTest)
{
    // Depth Configuration
    Resolution r = {0};
    r.width = 640;
    r.height = 480;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 60;
    dP.streamType = StreamType::Depth_Stream;

    // Color Configuration
    Resolution cR = {0};
    cR.width = 640;
    cR.height = 480;
    Profile cP;
    cP.pixelFormat = V4L2_PIX_FMT_YUYV;
    cP.resolution = cR;
    cP.fps = 60;
    cP.streamType = StreamType::Color_Stream;

    configure(5, 50, false);
    run(dP, cP);
}