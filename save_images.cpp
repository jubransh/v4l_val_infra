#include <unistd.h>
using namespace std;

bool isCollectFrames = false;
string cameraSerialNumber = "";
string scriptPath = "";
Profile running_profile;

class SaveImagesScript : public testing::Test
{

protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

void write_to_bin_file(string filePath, uint8_t *ptr, size_t len)
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
void save_FrameArrived(Frame f)
{
    // if (isCollectFrames && f.ID >= 10)
    // {
        string fm = running_profile.GetFormatText();
        string w = to_string(running_profile.resolution.width);
        string h = to_string(running_profile.resolution.height);
        string fps = to_string(running_profile.fps);

        string fileName = fm + "_" + w + "_" + h + "_" + fps + "_" + to_string(f.ID) +".bin";
        string imagePath = File_Utils::join(scriptPath, fileName);
        write_to_bin_file(imagePath, f.Buff, running_profile.resolution.width * running_profile.resolution.height * running_profile.GetBpp());
        // isCollectFrames = false;
    // }
}

vector<vector<Profile>> get_profiles(vector<StreamType> streamTypes)
{
    ProfileGenerator pG;
    vector<vector<Profile>> combinations = pG.GetCombintaions(streamTypes);
    return combinations;
}

TEST_F(SaveImagesScript, SaveDepthImagesScript)
{
    cout << "=================================================" << endl;
    cout << "         Save Depth Images Script " << endl;
    cout << "=================================================" << endl;
    bool streaming_script_status = true;
    string timestamp = TimeUtils::getDateandTime();

    Camera cam;
    cam.Init();
    cameraSerialNumber = cam.GetSerialNumber();

    char tmp[256];
    getcwd(tmp, 256);
    string scriptBasePath(tmp);

    scriptPath = FileUtils::join(scriptBasePath, "Depth_Saved_images_"+timestamp);
    FileUtils::makePath(scriptPath);
    vector<StreamType> streams;
    streams.push_back(StreamType::Depth_Stream);

    vector<vector<Profile>> profiles = get_profiles(streams);

    auto depthSensor = cam.GetDepthSensor();
    depthSensor->copyFrameData = true;

    for (int j = 0; j < profiles.size(); j++)
    {
        for (int i = 0; i < profiles[j].size(); i++)
        {
            running_profile = profiles[j][i];
            depthSensor->Configure(profiles[j][i]);
            depthSensor->Start(save_FrameArrived);

            isCollectFrames = true;

            for (int i = 0; i < 100; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!isCollectFrames)
                    break;
            }

            depthSensor->Stop();
            depthSensor->Close();
        }
    }
}

TEST_F(SaveImagesScript, SaveIRImagesScript)
{
    cout << "=================================================" << endl;
    cout << "         Save IR Images Script " << endl;
    cout << "=================================================" << endl;
    bool streaming_script_status = true;
    string timestamp = TimeUtils::getDateandTime();

    Camera cam;
    cam.Init();
    cameraSerialNumber = cam.GetSerialNumber();

    char tmp[256];
    getcwd(tmp, 256);
    string scriptBasePath(tmp);

    scriptPath = FileUtils::join(scriptBasePath, "IR_Saved_images_"+timestamp);
    FileUtils::makePath(scriptPath);
    vector<StreamType> streams;
    streams.push_back(StreamType::IR_Stream);

    vector<vector<Profile>> profiles = get_profiles(streams);

    auto irSensor = cam.GetIRSensor();
    irSensor->copyFrameData = true;

    for (int j = 0; j < profiles.size(); j++)
    {
        for (int i = 0; i < profiles[j].size(); i++)
        {
            running_profile = profiles[j][i];
            irSensor->Configure(profiles[j][i]);
            irSensor->Start(save_FrameArrived);

            isCollectFrames = true;

            for (int i = 0; i < 1000; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!isCollectFrames)
                    break;
            }

            irSensor->Stop();
            irSensor->Close();
        }
    }
}


TEST_F(SaveImagesScript, SaveColorImagesScript)
{
    cout << "=================================================" << endl;
    cout << "         Save Color Images Script " << endl;
    cout << "=================================================" << endl;
    bool streaming_script_status = true;
    string timestamp = TimeUtils::getDateandTime();

    Camera cam;
    cam.Init();
    cameraSerialNumber = cam.GetSerialNumber();

    char tmp[256];
    getcwd(tmp, 256);
    string scriptBasePath(tmp);

    scriptPath = FileUtils::join(scriptBasePath, "Color_Saved_images_"+timestamp);
    FileUtils::makePath(scriptPath);
    vector<StreamType> streams;
    streams.push_back(StreamType::Color_Stream);

    vector<vector<Profile>> profiles = get_profiles(streams);

    auto colorSensor = cam.GetColorSensor();
    colorSensor->copyFrameData = true;

    for (int j = 0; j < profiles.size(); j++)
    {
        for (int i = 0; i < profiles[j].size(); i++)
        {
            running_profile = profiles[j][i];
            colorSensor->Configure(profiles[j][i]);
            colorSensor->Start(save_FrameArrived);

            isCollectFrames = true;

            for (int i = 0; i < 10; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                // if (!isCollectFrames)
                //     break;
            }

            colorSensor->Stop();
            colorSensor->Close();
            break;
        }
        break;
    }
}