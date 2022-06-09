#include <unistd.h>
using namespace std;

vector<Frame> framesList;
bool collect_depth_Frames = false;
string cameraSerialNumber = "";
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
        collect_depth_Frames = false;
    }
}

TEST_F(TestersScripts, StreamingScript)
{
    cout << "=================================================" << endl;
    cout << "         Testers Streaming script " << endl;
    cout << "=================================================" << endl;
    bool streaming_script_status = true;

    Camera cam;
    cam.Init();
    cameraSerialNumber = cam.GetSerialNumber();

    char tmp[256];
    getcwd(tmp, 256);
    string scriptBasePath(tmp);

    scriptPath = FileUtils::join(scriptBasePath, "Streaming_Script_Logs");
    FileUtils::makePath(scriptPath);

    HWMonitorCommand hmc = {0};

    // GPWM command
    hmc.dataSize = 0;
    hmc.opCode = 0x80;  // CUMSOM_CMD
    hmc.parameter1 = 7; // Test Pattern
    hmc.parameter2 = 1; // On

    auto depthSensor = cam.GetDepthSensor();
    depthSensor->copyFrameData = true;

    // Depth Configuration
    Resolution r = {0};
    r.width = 1280;
    r.height = 720;
    Profile dP;
    dP.pixelFormat = V4L2_PIX_FMT_Z16;
    dP.resolution = r;
    dP.fps = 30;

    depthSensor->Configure(dP);

    depthSensor->Start(save_depthFrameArrived);

    std::this_thread::sleep_for(std::chrono::seconds(4));

    cout << "Enabled Pattern" << endl;
    auto cResult = cam.SendHWMonitorCommand(hmc);
    cout << "command result: " << cResult.Result << endl;

    collect_depth_Frames = true;

    for (int i = 0; i < 1000; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!collect_depth_Frames)
            break;
    }

    depthSensor->Stop();
    depthSensor->Close();

    for (int i = 0; i < framesList.size(); i++)
    {
        string fileName = cameraSerialNumber + "_" + TimeUtils::getDateandTime() + ".bin";
        string imagePath = File_Utils::join(scriptPath, fileName);
        cout << "Folder path: " << scriptPath << endl;
        cout << "Image path: " << imagePath << endl;
        write_to_file(imagePath, framesList[i].Buff, framesList[i].size);
    }

    delete fileBuf;
}