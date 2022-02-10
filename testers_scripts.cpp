using namespace std;

vector<Frame> framesList;
bool collect_depth_Frames = false;
string cameraSerialNumber = "";
string scriptBasePath = "/home/nvidia/";
string scriptPath = "";
uint8_t *fileBuf;

long getFileSize(FILE *file)
{
    long lCurPos, lEndPos;
    lCurPos = ftell(file);
    fseek(file, 0, 2);
    lEndPos = ftell(file);
    fseek(file, lCurPos, 0);
    return lEndPos;
}

class TestersScripts : public testing::Test
{

protected:
    void SetUp() override
    {
        string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    }

    void TearDown() override
    {
    }
};

void write_to_file(string filePath, uint8_t *ptr, size_t len)
{
    try
    {
        ofstream fp;
        fp.open(filePath.c_str(), ios ::binary);
        fp.write((char *)ptr, len);
        fp.close();
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << endl;
    }
}

// implemetnt the callback
void save_depthFrameArrived(Frame f)
{
    if (collect_depth_Frames)
    {
        framesList.push_back(f);
    }
}

TEST_F(TestersScripts, StreamingScript)
{
    cout << "=================================================" << endl;
    cout << "         Testers Streaming script " << endl;
    cout << "=================================================" << endl;
    bool streaming_script_status = true;
    // char cwd[256];
    // getcwd(cwd, 256);

    // string known_image_path = File_Utils::join(cwd, "/known_image.bin");
    // FILE *file = NULL;
    // const char *filePath = known_image_path.c_str();

    // // Open the file in binary mode using the "rb" format string
    // // This also checks if the file exists and/or can be opened for reading correctly
    // if ((file = fopen(filePath, "rb")) == NULL)
    //     cout << "Failed to open the known image" << endl;
    // else
    //     cout << "Known image opened successfully" << endl;

    // // Get the size of the file in bytes
    // long fileSize = getFileSize(file);
    // // Allocate space in the buffer for the whole file
    // fileBuf = new uint8_t[fileSize];

    // // Read the file in to the buffer
    // fread(fileBuf, fileSize, 1, file);

    Camera cam;
    cam.Init();
    cameraSerialNumber = cam.GetSerialNumber();

    scriptPath = FileUtils::join(scriptBasePath, "Streaming_Script_Logs");
    scriptPath = FileUtils::join(scriptPath, cameraSerialNumber + "_" + TimeUtils::getDateandTime());
    FileUtils::makePath(scriptPath);

    HWMonitorCommand hmc = {0};

    // GPWM command
    hmc.dataSize = 0;
    hmc.opCode = 0x80;  // CUMSOM_CMD
    hmc.parameter1 = 7; // Test Pattern
    hmc.parameter2 = 1; // On

    auto depthSensor = cam.GetDepthSensor();
    depthSensor.copyFrameData = true;

    // Depth Configuration
    Resolution r = {0};
    r.width = 1280;
    r.height = 720;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 30;

    depthSensor.Configure(dP);

    depthSensor.Start(save_depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    cout << "Enabled Pattern" << endl;
    auto cResult = cam.SendHWMonitorCommand(hmc);
    cout << "command result: " << cResult.Result << endl;

    collect_depth_Frames = true;

    std::this_thread::sleep_for(std::chrono::seconds(4));

    collect_depth_Frames = false;

    depthSensor.Stop();
    depthSensor.Close();

    // framesList.clear();

    for (int i = 0; i < framesList.size(); i++)
    {
        string fileName = "depth_frame_" + to_string(framesList[i].ID) + ".bin";
        string imagePath = File_Utils::join(scriptPath, fileName);
        write_to_file(imagePath, framesList[i].Buff, framesList[i].size);
        // int is_equal;
        // is_equal = memcmp((char *)fileBuf, (char *)framesList[i].Buff, framesList[i].size);
        // cout << "is_equal" << is_equal << endl;
        // cout << "Size: " << framesList[i].size << endl;
        // if (is_equal != 0)
        // {
        //     streaming_script_status = false;
        //     cout << "Frame number: " << framesList[i].ID << " isn't equal to the known image" << endl;
        // }
    }

    delete fileBuf;
}