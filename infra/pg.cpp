// Example program
#include <iostream>
#include <string>
#include <vector>
using namespace std;

/*
struct Resolution
{
    uint32_t width;
    uint32_t height;
};

class Profile
{
private:
    int bpp;

public:
    uint32_t pixelFormat;
    Resolution resolution;
    uint32_t fps;
    std::string streamType;
};
*/
class ProfileGenerator
{

private:
    //Profiles
    vector<string> depth;
    vector<string> ir;
    vector<string> color;
    vector<string> depth_color;
    vector<string> ir_color;
    vector<string> depth_ir;
    vector<string> depth_ir_color;
    vector<string> imu;
    vector<string> depth_imu;
    vector<string> ir_color_imu;
    vector<string> depth_ir_color_imu;


    Profile ParseProfile(std::string profileStr)
    {
        Profile prof;
        /*  uint32_t pixelFormat;
            Resolution resolution;
            uint32_t fps;
            int streamType
        */

        std::string rest;
        std::string delimiter = "_";

        //Get the format String
        std::string formatStr = profileStr.substr(0, profileStr.find(delimiter)); // token is "scott"
        rest = profileStr.substr(profileStr.find(delimiter) + 1, profileStr.length() - 1);

        //Get the resolution string 
        std::string resolutionStr = rest.substr(0, rest.find(delimiter));
        rest = rest.substr(rest.find(delimiter) + 1, rest.length() - 1);

        //Get the fps string
        std::string fpsStr = rest.substr(0, rest.find(delimiter)); // token is "scott"

        //extrac the resolutio string to width & hight
        std::string width = resolutionStr.substr(0, resolutionStr.find("x"));
        std::string height = resolutionStr.substr(resolutionStr.find("x") + 1, resolutionStr.length() - 1);


        Resolution r;
        r.width = static_cast<uint32_t>(std::stoul(width));
        r.height = static_cast<uint32_t>(std::stoul(height));
        prof.fps = static_cast<uint32_t>(std::stoul(fpsStr));
        prof.resolution = r;

        if (formatStr=="y8")
            {
            // prof.pixelFormat = V4L2_PIX_FMT_Y8;
            prof.pixelFormat = V4L2_PIX_FMT_GREY;
            
            prof.streamType = StreamType::IR_Stream;
            }
        else if (formatStr == "y8i")
        {
            prof.pixelFormat = V4L2_PIX_FMT_Y8I;
            prof.streamType = StreamType::IR_Stream;
        }
        else if (formatStr=="y12i")
            { 
            prof.pixelFormat = V4L2_PIX_FMT_Y12I;
            prof.streamType = StreamType::IR_Stream;
            }
        else if (formatStr=="z16")
            {
            prof.pixelFormat =V4L2_PIX_FMT_Z16;
            prof.streamType = StreamType::Depth_Stream;
            }
        else if (formatStr=="yuyv")
            {
            prof.pixelFormat =V4L2_PIX_FMT_YUYV;
            prof.streamType = StreamType::Color_Stream;
            }
        else if (formatStr=="imu")
            {
            prof.pixelFormat = 0; //need to update with actual Accel pixel format
            prof.streamType = StreamType::Imu_Stream;
            }

        return prof;
    }

    vector<Profile> ParseProfiles(std::string profilesStr)
    {
        vector<Profile> profilesVect;

        std::string rest;
        std::string delimiter = "+";

        //Get the format String
        rest = profilesStr;
        while (rest.find(delimiter) != -1)
        {
            string profStr = rest.substr(0, rest.find(delimiter));
            rest = rest.substr(rest.find(delimiter) + 1, rest.length() - 1);

            Profile p = ParseProfile(profStr);
            profilesVect.push_back(p);

        }
        Profile p = ParseProfile(rest);
        profilesVect.push_back(p);

        return profilesVect;
    }

    vector<vector<Profile>> ParseProfilesFromVector(vector<string> combinationsList)
    {
        vector<vector<Profile>> ProfilesCombinations;
        for (int i = 0; i < combinationsList.size(); i++)
        {
            ProfilesCombinations.push_back(ParseProfiles(combinationsList[i]));
        }
        return ProfilesCombinations;
    }

    bool findInVector(StreamType strType, vector<StreamType> list)
    {        
        for (int i = 0; i < list.size(); i++)
        {
            if (strType== list[i])
            {
                return true;
            }
        }

        return false;
    }

public:

    ProfileGenerator()
    {
        //============ Depth Only =================
        depth.push_back("z16_424x240_5");
        depth.push_back("z16_424x240_15");
        depth.push_back("z16_424x240_30");
        depth.push_back("z16_424x240_60");
        depth.push_back("z16_424x240_90");

        depth.push_back("z16_640x480_5");
        depth.push_back("z16_640x480_15");
        depth.push_back("z16_640x480_30");
        depth.push_back("z16_640x480_60");
        depth.push_back("z16_640x480_90");

        
        depth.push_back("z16_848x480_5");
        depth.push_back("z16_848x480_15");
        depth.push_back("z16_848x480_30");
        depth.push_back("z16_848x480_60");
        depth.push_back("z16_848x480_90");

        depth.push_back("z16_1280x720_5");
        depth.push_back("z16_1280x720_15");
        depth.push_back("z16_1280x720_30");

        //============ IR Only ================
            //  Y8
        ir.push_back("y8_424x240_5");
        ir.push_back("y8_424x240_15");
        ir.push_back("y8_424x240_30");
        ir.push_back("y8_424x240_60");
        ir.push_back("y8_424x240_90");

        ir.push_back("y8_640x480_5");
        ir.push_back("y8_640x480_15");
        ir.push_back("y8_640x480_30");
        ir.push_back("y8_640x480_60");
        ir.push_back("y8_640x480_90");

        ir.push_back("y8_848x480_5");
        ir.push_back("y8_848x480_15");
        ir.push_back("y8_848x480_30");
        ir.push_back("y8_848x480_60");
        ir.push_back("y8_848x480_90");

        ir.push_back("y8_1280x720_5");
        ir.push_back("y8_1280x720_15");
        ir.push_back("y8_1280x720_30");

            // Y8i
        ir.push_back("y8i_424x240_5");
        ir.push_back("y8i_424x240_15");
        ir.push_back("y8i_424x240_30");
        ir.push_back("y8i_424x240_60");
        ir.push_back("y8i_424x240_90");

        ir.push_back("y8i_640x480_5");
        ir.push_back("y8i_640x480_15");
        ir.push_back("y8i_640x480_30");
        ir.push_back("y8i_640x480_60");
        ir.push_back("y8i_640x480_90");

        ir.push_back("y8i_848x480_5");
        ir.push_back("y8i_848x480_15");
        ir.push_back("y8i_848x480_30");
        ir.push_back("y8i_848x480_60");
        ir.push_back("y8i_848x480_90");

        ir.push_back("y8i_1280x720_5");
        ir.push_back("y8i_1280x720_15");
        ir.push_back("y8i_1280x720_30");

        //Y12i
        ir.push_back("y12i_1280x800_15");
        ir.push_back("y12i_1280x800_30");

        //============ Color Only =================
        color.push_back("yuyv_424x240_5");
        color.push_back("yuyv_424x240_15");
        color.push_back("yuyv_424x240_30");
        color.push_back("yuyv_424x240_60");
        color.push_back("yuyv_424x240_90");


        color.push_back("yuyv_640x480_5");
        color.push_back("yuyv_640x480_15");
        color.push_back("yuyv_640x480_30");
        color.push_back("yuyv_640x480_60");


        color.push_back("yuyv_848x480_5");
        color.push_back("yuyv_848x480_15");
        color.push_back("yuyv_848x480_30");
        color.push_back("yuyv_848x480_60");


        color.push_back("yuyv_1280x720_5");
        color.push_back("yuyv_1280x720_15");
        color.push_back("yuyv_1280x720_30");

        color.push_back("yuyv_1280x800_5");
        color.push_back("yuyv_1280x800_15");
        color.push_back("yuyv_1280x800_30");

        //============ IMU =================
        imu.push_back("imu_0x0_50");
        imu.push_back("imu_0x0_100");
        imu.push_back("imu_0x0_200");
        imu.push_back("imu_0x0_400");


        //============ Depth +IR =================
            //  Depth + IR Y8
        depth_ir.push_back("z16_424x240_5+y8_424x240_5");
        depth_ir.push_back("z16_424x240_15+y8_424x240_15");
        depth_ir.push_back("z16_424x240_30+y8_424x240_30");
        depth_ir.push_back("z16_424x240_60+y8_424x240_60");
        depth_ir.push_back("z16_424x240_90+y8_424x240_90");


        depth_ir.push_back("z16_640x480_5+y8_640x480_5");
        depth_ir.push_back("z16_640x480_15+y8_640x480_15");
        depth_ir.push_back("z16_640x480_30+y8_640x480_30");
        depth_ir.push_back("z16_640x480_60+y8_640x480_60");
        depth_ir.push_back("z16_640x480_90+y8_640x480_90");

        depth_ir.push_back("z16_848x480_5+y8_848x480_5");
        depth_ir.push_back("z16_848x480_15+y8_848x480_15");
        depth_ir.push_back("z16_848x480_30+y8_848x480_30");
        depth_ir.push_back("z16_848x480_60+y8_848x480_60");
        depth_ir.push_back("z16_848x480_90+y8_848x480_90");

        depth_ir.push_back("z16_1280x720_5+y8_1280x720_5");
        depth_ir.push_back("z16_1280x720_15+y8_1280x720_15");
        depth_ir.push_back("z16_1280x720_30+y8_1280x720_30");

            // Depth + IR Y8i
        depth_ir.push_back("z16_424x240_5+y8i_424x240_5");
        depth_ir.push_back("z16_424x240_15+y8i_424x240_15");
        depth_ir.push_back("z16_424x240_30+y8i_424x240_30");
        depth_ir.push_back("z16_424x240_60+y8i_424x240_60");
        depth_ir.push_back("z16_424x240_90+y8i_424x240_90");


        depth_ir.push_back("z16_640x480_5+y8i_640x480_5");
        depth_ir.push_back("z16_640x480_15+y8i_640x480_15");
        depth_ir.push_back("z16_640x480_30+y8i_640x480_30");
        depth_ir.push_back("z16_640x480_60+y8i_640x480_60");
        depth_ir.push_back("z16_640x480_90+y8i_640x480_90");

        depth_ir.push_back("z16_848x480_5+y8i_848x480_5");
        depth_ir.push_back("z16_848x480_15+y8i_848x480_15");
        depth_ir.push_back("z16_848x480_30+y8i_848x480_30");
        depth_ir.push_back("z16_848x480_60+y8i_848x480_60");
        depth_ir.push_back("z16_848x480_90+y8i_848x480_90");

        depth_ir.push_back("z16_1280x720_5+y8i_1280x720_5");
        depth_ir.push_back("z16_1280x720_15+y8i_1280x720_15");
        depth_ir.push_back("z16_1280x720_30+y8i_1280x720_30");

        //============ Depth + Color =================
        //  depth_color.push_back("z16_424x240_5+yuyv_424x240_5");
        //  depth_color.push_back("z16_424x240_15+yuyv_424x240_15");
        //  depth_color.push_back("z16_424x240_30+yuyv_424x240_30");
        //  depth_color.push_back("z16_424x240_60+yuyv_424x240_60");
        //  depth_color.push_back("z16_424x240_90+yuyv_424x240_90");


        // depth_color.push_back("z16_640x480_5+yuyv_640x480_5");
        // depth_color.push_back("z16_640x480_15+yuyv_640x480_15");
        depth_color.push_back("z16_640x480_30+yuyv_640x480_30");
        // depth_color.push_back("z16_640x480_60+yuyv_640x480_60");
        // depth_color.push_back("z16_640x480_90+yuyv_640x480_90");
        

        // depth_color.push_back("z16_1280x720_5+yuyv_1280x720_5");
        // depth_color.push_back("z16_1280x720_15+yuyv_1280x720_15");
        // depth_color.push_back("z16_1280x720_30+yuyv_1280x720_30");

        //============ IR + Color =================
            //Y8
        ir_color.push_back("y8_424x240_5+yuyv_424x240_5");
        ir_color.push_back("y8_424x240_15+yuyv_424x240_15");
        ir_color.push_back("y8_424x240_30+yuyv_424x240_30");
        ir_color.push_back("y8_424x240_60+yuyv_424x240_60");
        ir_color.push_back("y8_424x240_90+yuyv_424x240_90");


        ir_color.push_back("y8_640x480_5+yuyv_640x480_5");
        ir_color.push_back("y8_640x480_15+yuyv_640x480_15");
        ir_color.push_back("y8_640x480_30+yuyv_640x480_30");
        ir_color.push_back("y8_640x480_60+yuyv_640x480_60");
        

        ir_color.push_back("y8_1280x720_5+yuyv_1280x720_5");
        ir_color.push_back("y8_1280x720_15+yuyv_1280x720_15");
        ir_color.push_back("y8_1280x720_30+yuyv_1280x720_30");

            //Y8i
        ir_color.push_back("y8i_424x240_5+yuyv_424x240_5");
        ir_color.push_back("y8i_424x240_15+yuyv_424x240_15");
        ir_color.push_back("y8i_424x240_30+yuyv_424x240_30");
        ir_color.push_back("y8i_424x240_60+yuyv_424x240_60");
        ir_color.push_back("y8i_424x240_90+yuyv_424x240_90");


        ir_color.push_back("y8i_640x480_5+yuyv_640x480_5");
        ir_color.push_back("y8i_640x480_15+yuyv_640x480_15");
        ir_color.push_back("y8i_640x480_30+yuyv_640x480_30");
        ir_color.push_back("y8i_640x480_60+yuyv_640x480_60");


        ir_color.push_back("y8i_1280x720_5+yuyv_1280x720_5");
        ir_color.push_back("y8i_1280x720_15+yuyv_1280x720_15");
        ir_color.push_back("y8i_1280x720_30+yuyv_1280x720_30");

            //Y12i
        ir_color.push_back("y12i_1280x800_15+yuyv_1280x720_30");
        ir_color.push_back("y12i_1280x800_30+yuyv_1280x720_30");

        //============ Depth + IR + Color =================
            //Y8
        // depth_ir_color.push_back("z16_424x240_5+y8_424x240_5+yuyv_424x240_5");
        // depth_ir_color.push_back("z16_424x240_15+y8_424x240_15+yuyv_424x240_15");
        // depth_ir_color.push_back("z16_424x240_30+y8_424x240_30+yuyv_424x240_30");
        // depth_ir_color.push_back("z16_424x240_60+y8_424x240_60+yuyv_424x240_60");
        // depth_ir_color.push_back("z16_424x240_90+y8_424x240_90+yuyv_424x240_90");


        depth_ir_color.push_back("z16_640x480_5+y8_640x480_5+yuyv_640x480_5");
        depth_ir_color.push_back("z16_640x480_15+y8_640x480_15+yuyv_640x480_15");
        depth_ir_color.push_back("z16_640x480_30+y8_640x480_30+yuyv_640x480_30");
        depth_ir_color.push_back("z16_640x480_60+y8_640x480_60+yuyv_640x480_60");
        
        // depth_ir_color.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_640x480_30");

        depth_ir_color.push_back("z16_1280x720_5+y8_1280x720_5+yuyv_1280x720_5");
        depth_ir_color.push_back("z16_1280x720_15+y8_1280x720_15+yuyv_1280x720_15");
        depth_ir_color.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_1280x720_30");

        //     //Y8i

        // depth_ir_color.push_back("z16_424x240_5+y8i_424x240_5+yuyv_424x240_5");
        // depth_ir_color.push_back("z16_424x240_15+y8i_424x240_15+yuyv_424x240_15");
        // depth_ir_color.push_back("z16_424x240_30+y8i_424x240_30+yuyv_424x240_30");
        // depth_ir_color.push_back("z16_424x240_60+y8i_424x240_60+yuyv_424x240_60");
        // depth_ir_color.push_back("z16_424x240_90+y8i_424x240_90+yuyv_424x240_90");


        // depth_ir_color.push_back("z16_640x480_5+y8i_640x480_5+yuyv_640x480_5");
        // depth_ir_color.push_back("z16_640x480_15+y8i_640x480_15+yuyv_640x480_15");
        // depth_ir_color.push_back("z16_640x480_30+y8i_640x480_30+yuyv_640x480_30");
        // depth_ir_color.push_back("z16_640x480_60+y8i_640x480_60+yuyv_640x480_60");

        // depth_ir_color.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_640x480_30");

        // depth_ir_color.push_back("z16_1280x720_5+y8i_1280x720_5+yuyv_1280x720_5");
        // depth_ir_color.push_back("z16_1280x720_15+y8i_1280x720_15+yuyv_1280x720_15");
        // depth_ir_color.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_1280x720_30");

        //============ Depth + IMU =================
        depth_imu.push_back("z16_640x480_5+imu_0x0_50");
        depth_imu.push_back("z16_640x480_15+imu_0x0_50");
        depth_imu.push_back("z16_640x480_30+imu_0x0_50");
        depth_imu.push_back("z16_640x480_60+imu_0x0_50");
        depth_imu.push_back("z16_640x480_90+imu_0x0_50");
        depth_imu.push_back("z16_1280x720_5+imu_0x0_50");
        depth_imu.push_back("z16_1280x720_15+imu_0x0_50");
        depth_imu.push_back("z16_1280x720_30+imu_0x0_50");

        depth_imu.push_back("z16_640x480_5+imu_0x0_100");
        depth_imu.push_back("z16_640x480_15+imu_0x0_100");
        depth_imu.push_back("z16_640x480_30+imu_0x0_100");
        depth_imu.push_back("z16_640x480_60+imu_0x0_100");
        depth_imu.push_back("z16_640x480_90+imu_0x0_100");
        depth_imu.push_back("z16_1280x720_5+imu_0x0_100");
        depth_imu.push_back("z16_1280x720_15+imu_0x0_100");
        depth_imu.push_back("z16_1280x720_30+imu_0x0_100");

        depth_imu.push_back("z16_640x480_5+imu_0x0_200");
        depth_imu.push_back("z16_640x480_15+imu_0x0_200");
        depth_imu.push_back("z16_640x480_30+imu_0x0_200");
        depth_imu.push_back("z16_640x480_60+imu_0x0_200");
        depth_imu.push_back("z16_640x480_90+imu_0x0_200");
        depth_imu.push_back("z16_1280x720_5+imu_0x0_200");
        depth_imu.push_back("z16_1280x720_15+imu_0x0_200");
        depth_imu.push_back("z16_1280x720_30+imu_0x0_200");

        depth_imu.push_back("z16_640x480_5+imu_0x0_400");
        depth_imu.push_back("z16_640x480_15+imu_0x0_400");
        depth_imu.push_back("z16_640x480_30+imu_0x0_400");
        depth_imu.push_back("z16_640x480_60+imu_0x0_400");
        depth_imu.push_back("z16_640x480_90+imu_0x0_400");
        depth_imu.push_back("z16_1280x720_5+imu_0x0_400");
        depth_imu.push_back("z16_1280x720_15+imu_0x0_400");
        depth_imu.push_back("z16_1280x720_30+imu_0x0_400");

        //============ IR + Color  + IMU =================
            //Y8
        ir_color_imu.push_back("y8_640x480_5+yuyv_640x480_5+imu_0x0_50");
        ir_color_imu.push_back("y8_640x480_15+yuyv_640x480_15+imu_0x0_50");
        ir_color_imu.push_back("y8_640x480_30+yuyv_640x480_30+imu_0x0_50");
        ir_color_imu.push_back("y8_640x480_60+yuyv_640x480_60+imu_0x0_50");
        ir_color_imu.push_back("y8_1280x720_5+yuyv_1280x720_5+imu_0x0_50");
        ir_color_imu.push_back("y8_1280x720_15+yuyv_1280x720_15+imu_0x0_50");
        ir_color_imu.push_back("y8_1280x720_30+yuyv_1280x720_30+imu_0x0_50");
        
        ir_color_imu.push_back("y8_640x480_5+yuyv_640x480_5+imu_0x0_100");
        ir_color_imu.push_back("y8_640x480_15+yuyv_640x480_15+imu_0x0_100");
        ir_color_imu.push_back("y8_640x480_30+yuyv_640x480_30+imu_0x0_100");
        ir_color_imu.push_back("y8_640x480_60+yuyv_640x480_60+imu_0x0_100");
        ir_color_imu.push_back("y8_1280x720_5+yuyv_1280x720_5+imu_0x0_100");
        ir_color_imu.push_back("y8_1280x720_15+yuyv_1280x720_15+imu_0x0_100");
        ir_color_imu.push_back("y8_1280x720_30+yuyv_1280x720_30+imu_0x0_100");

        ir_color_imu.push_back("y8_640x480_5+yuyv_640x480_5+imu_0x0_200");
        ir_color_imu.push_back("y8_640x480_15+yuyv_640x480_15+imu_0x0_200");
        ir_color_imu.push_back("y8_640x480_30+yuyv_640x480_30+imu_0x0_200");
        ir_color_imu.push_back("y8_640x480_60+yuyv_640x480_60+imu_0x0_200");
        ir_color_imu.push_back("y8_1280x720_5+yuyv_1280x720_5+imu_0x0_200");
        ir_color_imu.push_back("y8_1280x720_15+yuyv_1280x720_15+imu_0x0_200");
        ir_color_imu.push_back("y8_1280x720_30+yuyv_1280x720_30+imu_0x0_200");   

        ir_color_imu.push_back("y8_640x480_5+yuyv_640x480_5+imu_0x0400");
        ir_color_imu.push_back("y8_640x480_15+yuyv_640x480_15+imu_0x0_400");
        ir_color_imu.push_back("y8_640x480_30+yuyv_640x480_30+imu_0x0_400");
        ir_color_imu.push_back("y8_640x480_60+yuyv_640x480_60+imu_0x0_400");
        ir_color_imu.push_back("y8_1280x720_5+yuyv_1280x720_5+imu_0x0_400");
        ir_color_imu.push_back("y8_1280x720_15+yuyv_1280x720_15+imu_0x0_400");
        ir_color_imu.push_back("y8_1280x720_30+yuyv_1280x720_30+imu_0x0_400");

            //Y8i
        ir_color_imu.push_back("y8i_640x480_5+yuyv_640x480_5+imu_0x0_50");
        ir_color_imu.push_back("y8i_640x480_15+yuyv_640x480_15+imu_0x0_50");
        ir_color_imu.push_back("y8i_640x480_30+yuyv_640x480_30+imu_0x0_50");
        ir_color_imu.push_back("y8i_640x480_60+yuyv_640x480_60+imu_0x0_50");
        ir_color_imu.push_back("y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_50");
        ir_color_imu.push_back("y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_50");
        ir_color_imu.push_back("y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_50");

        ir_color_imu.push_back("y8i_640x480_5+yuyv_640x480_5+imu_0x0_100");
        ir_color_imu.push_back("y8i_640x480_15+yuyv_640x480_15+imu_0x0_100");
        ir_color_imu.push_back("y8i_640x480_30+yuyv_640x480_30+imu_0x0_100");
        ir_color_imu.push_back("y8i_640x480_60+yuyv_640x480_60+imu_0x0_100");
        ir_color_imu.push_back("y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_100");
        ir_color_imu.push_back("y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_100");
        ir_color_imu.push_back("y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_100");

        ir_color_imu.push_back("y8i_640x480_5+yuyv_640x480_5+imu_0x0_200");
        ir_color_imu.push_back("y8i_640x480_15+yuyv_640x480_15+imu_0x0_200");
        ir_color_imu.push_back("y8i_640x480_30+yuyv_640x480_30+imu_0x0_200");
        ir_color_imu.push_back("y8i_640x480_60+yuyv_640x480_60+imu_0x0_200");
        ir_color_imu.push_back("y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_200");
        ir_color_imu.push_back("y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_200");
        ir_color_imu.push_back("y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_200");

        ir_color_imu.push_back("y8i_640x480_5+yuyv_640x480_5+imu_0x0400");
        ir_color_imu.push_back("y8i_640x480_15+yuyv_640x480_15+imu_0x0_400");
        ir_color_imu.push_back("y8i_640x480_30+yuyv_640x480_30+imu_0x0_400");
        ir_color_imu.push_back("y8i_640x480_60+yuyv_640x480_60+imu_0x0_400");
        ir_color_imu.push_back("y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_400");
        ir_color_imu.push_back("y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_400");
        ir_color_imu.push_back("y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_400");
        
        //Y12i
        ir_color_imu.push_back("y12i_1280x800_15+yuyv_1280x720_30+imu_0x0_50");
        ir_color_imu.push_back("y12i_1280x800_30+yuyv_1280x720_30+imu_0x0_50");

        ir_color_imu.push_back("y12i_1280x800_15+yuyv_1280x720_30+imu_0x0_100");
        ir_color_imu.push_back("y12i_1280x800_30+yuyv_1280x720_30+imu_0x0_100");

        ir_color_imu.push_back("y12i_1280x800_15+yuyv_1280x720_30+imu_0x0_200");
        ir_color_imu.push_back("y12i_1280x800_30+yuyv_1280x720_30+imu_0x0_200");

        ir_color_imu.push_back("y12i_1280x800_15+yuyv_1280x720_30+imu_0x0_400");
        ir_color_imu.push_back("y12i_1280x800_30+yuyv_1280x720_30+imu_0x0_400");

        //============ Depth + IR + Color + IMU =================
            // IR Y8
        depth_ir_color_imu.push_back("z16_640x480_5+y8_640x480_5+yuyv_640x480_5+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_640x480_15+y8_640x480_15+yuyv_640x480_15+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_640x480_30+y8_640x480_30+yuyv_640x480_30+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_640x480_60+y8_640x480_60+yuyv_640x480_60+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_640x480_30+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8_1280x720_5+yuyv_1280x720_5+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8_1280x720_15+yuyv_1280x720_15+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_1280x720_30+imu_0x0_50");
        
        depth_ir_color_imu.push_back("z16_640x480_5+y8_640x480_5+yuyv_640x480_5+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_640x480_15+y8_640x480_15+yuyv_640x480_15+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_640x480_30+y8_640x480_30+yuyv_640x480_30+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_640x480_60+y8_640x480_60+yuyv_640x480_60+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_640x480_30+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8_1280x720_5+yuyv_1280x720_5+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8_1280x720_15+yuyv_1280x720_15+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_1280x720_30+imu_0x0_100");


        depth_ir_color_imu.push_back("z16_640x480_5+y8_640x480_5+yuyv_640x480_5+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_640x480_15+y8_640x480_15+yuyv_640x480_15+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_640x480_30+y8_640x480_30+yuyv_640x480_30+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_640x480_60+y8_640x480_60+yuyv_640x480_60+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_640x480_30+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8_1280x720_5+yuyv_1280x720_5+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8_1280x720_15+yuyv_1280x720_15+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_1280x720_30+imu_0x0_200");

        depth_ir_color_imu.push_back("z16_640x480_5+y8_640x480_5+yuyv_640x480_5+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_640x480_15+y8_640x480_15+yuyv_640x480_15+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_640x480_30+y8_640x480_30+yuyv_640x480_30+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_640x480_60+y8_640x480_60+yuyv_640x480_60+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_640x480_30+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8_1280x720_5+yuyv_1280x720_5+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8_1280x720_15+yuyv_1280x720_15+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8_1280x720_30+yuyv_1280x720_30+imu_0x0_400");



            // IR Y8I
        depth_ir_color_imu.push_back("z16_640x480_5+y8i_640x480_5+yuyv_640x480_5+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_640x480_15+y8i_640x480_15+yuyv_640x480_15+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_640x480_30+y8i_640x480_30+yuyv_640x480_30+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_640x480_60+y8i_640x480_60+yuyv_640x480_60+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_640x480_30+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_50");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_50");

        depth_ir_color_imu.push_back("z16_640x480_5+y8i_640x480_5+yuyv_640x480_5+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_640x480_15+y8i_640x480_15+yuyv_640x480_15+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_640x480_30+y8i_640x480_30+yuyv_640x480_30+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_640x480_60+y8i_640x480_60+yuyv_640x480_60+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_640x480_30+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_100");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_100");

        depth_ir_color_imu.push_back("z16_640x480_5+y8i_640x480_5+yuyv_640x480_5+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_640x480_15+y8i_640x480_15+yuyv_640x480_15+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_640x480_30+y8i_640x480_30+yuyv_640x480_30+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_640x480_60+y8i_640x480_60+yuyv_640x480_60+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_640x480_30+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_200");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_200");


        depth_ir_color_imu.push_back("z16_640x480_5+y8i_640x480_5+yuyv_640x480_5+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_640x480_15+y8i_640x480_15+yuyv_640x480_15+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_640x480_30+y8i_640x480_30+yuyv_640x480_30+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_640x480_60+y8i_640x480_60+yuyv_640x480_60+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_640x480_30+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_5+y8i_1280x720_5+yuyv_1280x720_5+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_15+y8i_1280x720_15+yuyv_1280x720_15+imu_0x0_400");
        depth_ir_color_imu.push_back("z16_1280x720_30+y8i_1280x720_30+yuyv_1280x720_30+imu_0x0_400");


    }


    vector<vector<Profile>> GetCombintaions(vector<StreamType> streamTypes)
    {
        int len = streamTypes.size();
        switch (len)
        {
        case 1:
        {
            if (findInVector(StreamType::Depth_Stream, streamTypes))
            {
                return ParseProfilesFromVector(depth);
            }
            if (findInVector(StreamType::IR_Stream, streamTypes))
            {
                return ParseProfilesFromVector(ir);
            }
            if (findInVector(StreamType::Color_Stream, streamTypes))
            {
                return ParseProfilesFromVector(color);
            }
            if (findInVector(StreamType::Imu_Stream, streamTypes))
            {
                return ParseProfilesFromVector(imu);
            }
            
            break;
        }
        case 2:
        {
            if (findInVector(StreamType::Depth_Stream, streamTypes) && findInVector(StreamType::Color_Stream, streamTypes))
            {
                return ParseProfilesFromVector(depth_color);
            }
            else if (findInVector(StreamType::IR_Stream, streamTypes) && findInVector(StreamType::Color_Stream, streamTypes))
            {
                return ParseProfilesFromVector(ir_color);
            }
            else if (findInVector(StreamType::IR_Stream, streamTypes) && findInVector(StreamType::Depth_Stream, streamTypes))
            {
                return ParseProfilesFromVector(depth_ir);
            }
            else if (findInVector(StreamType::Imu_Stream, streamTypes) && findInVector(StreamType::Depth_Stream, streamTypes))
            {
                return ParseProfilesFromVector(depth_imu);
            }
            break;
        }
        case 3:
        {
            if (findInVector(StreamType::Depth_Stream, streamTypes) && findInVector(StreamType::IR_Stream, streamTypes) && findInVector(StreamType::Color_Stream, streamTypes))
            {
                return ParseProfilesFromVector(depth_ir_color);
            }
            else if (findInVector(StreamType::Color_Stream, streamTypes) && findInVector(StreamType::IR_Stream, streamTypes) && findInVector(StreamType::Imu_Stream, streamTypes))
            {
                return ParseProfilesFromVector(ir_color_imu);
            }
            break;
        }
        case 4:
        {
            if (findInVector(StreamType::IR_Stream, streamTypes) && findInVector(StreamType::Color_Stream, streamTypes) && findInVector(StreamType::Depth_Stream, streamTypes) && findInVector(StreamType::Imu_Stream, streamTypes))
            {
                return ParseProfilesFromVector(depth_ir_color_imu);
            }
            break;
        }
        default: break;
        }
    }

    vector<vector<Profile>> GetMixedCombintaions(vector<StreamType> streamTypes)
    {
        vector<vector<Profile>> startingMix;
        vector<vector<Profile>> additionalMix;
        vector<vector<Profile>> finalMix;
        if (findInVector(StreamType::IR_Stream, streamTypes) && findInVector(StreamType::Depth_Stream, streamTypes))
            {
                startingMix= ParseProfilesFromVector(depth_ir);
            }
        else if (findInVector(StreamType::Depth_Stream, streamTypes))
            {
                startingMix= ParseProfilesFromVector(depth);
            }
        else if (findInVector(StreamType::IR_Stream, streamTypes))
            {
                startingMix= ParseProfilesFromVector(ir);
            }

        if (findInVector(StreamType::Color_Stream, streamTypes)) 
            {
                additionalMix = ParseProfilesFromVector(color);
            }
        for (int i=0; i< startingMix.size();i++)
        {
            for (int j=0; j<additionalMix.size();j++)
            {
                vector<Profile> prof;
                prof = startingMix[i];
                prof.push_back(additionalMix[j][0]);
                finalMix.push_back(prof);
            }
        }
        startingMix = finalMix;
        if (findInVector(StreamType::Imu_Stream, streamTypes))
            {
                additionalMix = ParseProfilesFromVector(imu);
            }
        else
            {
            return finalMix;
            }
        finalMix.clear();
        for (int i=0; i< startingMix.size();i++)
        {
            for (int j=0; j<additionalMix.size();j++)
            {
                vector<Profile> prof;
                prof = startingMix[i];
                prof.push_back(additionalMix[j][0]);
                finalMix.push_back(prof);
            }
        }
        return finalMix;


    }
    vector<Profile> GetHighestCombination(vector<StreamType> streamTypes)
    {
        vector<Profile> highestProfileCombination;
        double highestBandwidth=0;
        vector<vector<Profile>> allCombinations=GetCombintaions(streamTypes);
        for (int i =0; i< allCombinations.size();i++)
        {
            double currbandwidth =0;
            for (int j=0; j< allCombinations[i].size();j++)
            {
                currbandwidth+=allCombinations[i][j].GetSize() * allCombinations[i][j].fps;
            }
            if (currbandwidth>=highestBandwidth)
            {
                highestBandwidth=currbandwidth;
                highestProfileCombination = allCombinations[i];
            }
        }
        return highestProfileCombination;
    }

    vector<Profile> GetHighestCombination(vector<StreamType> streamTypes, int fps)
    {
        vector<Profile> highestProfileCombination;
        double highestBandwidth = 0;
        vector<vector<Profile>> allCombinations = GetCombintaions(streamTypes);
        for (int i = 0; i < allCombinations.size(); i++)
        {
            if (allCombinations[i][0].fps != fps)
                continue;
            double currbandwidth = 0;
            for (int j = 0; j < allCombinations[i].size(); j++)
            {
                currbandwidth += allCombinations[i][j].GetSize() * allCombinations[i][j].fps;
            }
            if (currbandwidth >= highestBandwidth)
            {
                highestBandwidth = currbandwidth;
                highestProfileCombination = allCombinations[i];
            }
        }
        return highestProfileCombination;
    }

    vector<Profile> GetProfilesByString(std::string profilesStr)
    {
        return ParseProfiles(profilesStr);
    }
    Resolution GetHighestResolution(StreamType streamType)
    {
        Resolution  highestResolution;
        double highestBandwidth = 0;
        vector<StreamType> t;
        t.push_back(streamType);
        vector<vector<Profile>> allCombinations = GetCombintaions(t);
        for (int i = 0; i < allCombinations.size(); i++)
        {
            double currbandwidth = 0;
            for (int j = 0; j < allCombinations[i].size(); j++)
            {
                currbandwidth += allCombinations[i][j].GetSize();
            }
            if (currbandwidth >= highestBandwidth)
            {
                highestBandwidth = currbandwidth;
                highestResolution = allCombinations[i][0].resolution;
            }
        }
        return highestResolution;
    }

    Resolution GetLowestResolution(StreamType streamType)
    {
        Resolution  lowestResolution;
        double lowestBandwidth = 0;
        vector<StreamType> t;
        t.push_back(streamType);
        vector<vector<Profile>> allCombinations = GetCombintaions(t);
        for (int i = 0; i < allCombinations.size(); i++)
        {
            double currbandwidth = 0;
            for (int j = 0; j < allCombinations[i].size(); j++)
            {
                currbandwidth += allCombinations[i][j].GetSize();
            }
            if (currbandwidth <= lowestBandwidth || lowestBandwidth ==0)
            {
                lowestBandwidth = currbandwidth;
                lowestResolution = allCombinations[i][0].resolution;
            }
        }
        return lowestResolution;
    }

    vector<Profile> GetControlsProfiles(StreamType streamType)
    {
        vector<Profile> controlsProfiles;
        Resolution highest, lowest;
        highest = GetHighestResolution(streamType);
        lowest = GetLowestResolution(streamType);
        vector<StreamType> streamTypes;
        streamTypes.push_back(streamType);
        vector<vector<Profile>> allProfiles = GetCombintaions(streamTypes);
        for (int i = 0; i < allProfiles.size(); i++)
        {
            if (allProfiles[i][0].resolution.width == highest.width && allProfiles[i][0].resolution.height == highest.height)
            {
                controlsProfiles.push_back(allProfiles[i][0]);
            }
            else if (allProfiles[i][0].resolution.width == lowest.width && allProfiles[i][0].resolution.height == lowest.height)
            {
                controlsProfiles.push_back(allProfiles[i][0]);
            }
        }
        return controlsProfiles;
    }
};


/*
int main()
{
    ProfileGenerator pG;
    vector<StreamType> streamTypes;
    streamTypes.push_back(StreamType::Color_Stream);
    // streamTypes.push_back("ir");
    streamTypes.push_back(StreamType::Depth_Stream);


    // auto p = ParseProfile("z16_640x480_30");
    // std::cout << "Profile: " << std::endl;
    // std::cout << "fps: " << p.fps << std::endl;
    auto combinations = pG.GetCombintaions(streamTypes);
    cout << combinations[2][1].fps;

    // 	cout << "sss";

}*/
