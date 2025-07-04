using namespace std;
// #include "CameraInfra.cpp"
#include <linux/v4l2-controls.h>
#include <string>
#include <vector>

class ControlConf
{
public:
    string _controlName;
    string _mDName;
    unsigned int _controlID;
    vector<int> _values;
    void PrittyPrint()
    {
        cout << "Control Name: " << _controlName << endl;
        cout << "MetaData Name: " << _mDName << endl;
        cout << "Control ID: " << _controlID << endl;
        cout << "Values are:" << endl;
        for (int i = 0; i < _values.size(); i++)
        {
            cout << _values[i] << endl;
        }
    }
};

class ControlsGenerator
{
public:
    static ControlConf get_control_conf(string controlName)
    {
        ControlConf conf;
        if (controlName == "Exposure")
        {
            conf._controlName = "Exposure";
            conf._controlID = V4L2_CID_EXPOSURE_ABSOLUTE;
            // conf._controlID = V4L2_CID_EXPOSURE;
            conf._mDName = "exposureTime";

            conf._values.push_back(1);
            conf._values.push_back(10020);
            conf._values.push_back(15520);
            conf._values.push_back(32220);
            conf._values.push_back(65520);            
            conf._values.push_back(165000);            
        }
        else if (controlName == "Gain")
        {
            conf._controlName = "Gain";
            conf._controlID = V4L2_CID_ANALOGUE_GAIN;
            conf._mDName = "Gain";
            conf._values.push_back(16);
            conf._values.push_back(74);
            conf._values.push_back(132);
            conf._values.push_back(190);
            conf._values.push_back(248);
        }
        else if (controlName == "LaserPower")
        {
            conf._controlName = "LaserPower";
            conf._controlID = DS5_CAMERA_CID_MANUAL_LASER_POWER;
            conf._mDName = "ManualLaserPower";
            conf._values.push_back(0);
            conf._values.push_back(60);
            conf._values.push_back(120);
            conf._values.push_back(180);
            conf._values.push_back(240);
            conf._values.push_back(300);
            conf._values.push_back(360);
        }
        else if (controlName == "LaserPowerMode")
        {
            conf._controlName = "LaserPowerMode";
            conf._controlID = DS5_CAMERA_CID_LASER_POWER;
            conf._mDName = "LaserPowerMode";
            conf._values.push_back(0);
            conf._values.push_back(1);
        }

        // Color Controls start here
        else if (controlName == "BackLighCompensation")
        {
            conf._controlName = "BackLighCompensation";
            conf._controlID = V4L2_CID_BACKLIGHT_COMPENSATION;
            conf._mDName = "BackLighCompensation";
            conf._values.push_back(0);
            conf._values.push_back(1);

        }
        else if (controlName == "Brightness")
        {
            conf._controlName = "Brightness";
            conf._controlID = V4L2_CID_BRIGHTNESS;
            conf._mDName = "Brightness";
            conf._values.push_back(-64);
            conf._values.push_back(-32);
            conf._values.push_back(0);
            conf._values.push_back(32);
            conf._values.push_back(64);
        }
        else if (controlName == "Contrast")
        {
            conf._controlName = "Contrast";
            conf._controlID = V4L2_CID_CONTRAST;
            conf._mDName = "Contrast";
            conf._values.push_back(0);
            conf._values.push_back(25);
            conf._values.push_back(50);
            conf._values.push_back(75);
            conf._values.push_back(100);
        }
        else if (controlName == "Color_Exposure")
        {
            conf._controlName = "Color_Exposure";
            conf._controlID = V4L2_CID_EXPOSURE_ABSOLUTE;
            conf._mDName = "exposureTime";
            conf._values.push_back(1);
            conf._values.push_back(2);
            conf._values.push_back(4);
            conf._values.push_back(9);
            conf._values.push_back(19);
            conf._values.push_back(39);
            conf._values.push_back(78);
            conf._values.push_back(156);
            conf._values.push_back(312);
            conf._values.push_back(625);
            conf._values.push_back(1250);
            conf._values.push_back(2500);
            conf._values.push_back(5000);
            conf._values.push_back(10000);
        }
        else if (controlName == "Color_Gain")
        {
            conf._controlName = "Color_Gain";
            conf._controlID = V4L2_CID_ANALOGUE_GAIN;
            conf._mDName = "Gain";
            conf._values.push_back(0);
            conf._values.push_back(32);
            conf._values.push_back(64);
            conf._values.push_back(96);
            conf._values.push_back(128);
        }
        else if (controlName == "Gamma")
        {
            conf._controlName = "Gamma";
            conf._controlID = V4L2_CID_GAMMA;
            conf._mDName = "Gamma";
            conf._values.push_back(100);
            conf._values.push_back(200);
            conf._values.push_back(300);
            conf._values.push_back(400);
            conf._values.push_back(500);
        }
        else if (controlName == "Hue")
        {
            conf._controlName = "Hue";
            conf._controlID = V4L2_CID_HUE;
            conf._mDName = "Hue";
            conf._values.push_back(-180);
            conf._values.push_back(-90);
            conf._values.push_back(0);
            conf._values.push_back(90);
            conf._values.push_back(180);
        }
        else if (controlName == "Saturation")
        {
            conf._controlName = "Saturation";
            conf._controlID = V4L2_CID_SATURATION ;
            conf._mDName = "Saturation";
            conf._values.push_back(0);
            conf._values.push_back(25);
            conf._values.push_back(50);
            conf._values.push_back(75);
            conf._values.push_back(100);
        }
        else if (controlName == "Sharpness")
        {
            conf._controlName = "Sharpness";
            conf._controlID = V4L2_CID_SHARPNESS ;
            conf._mDName = "Sharpness";
            conf._values.push_back(0);
            conf._values.push_back(25);
            conf._values.push_back(50);
            conf._values.push_back(75);
            conf._values.push_back(100);
        }
        else if (controlName == "WhiteBalance")
        {
            conf._controlName = "WhiteBalance";
            conf._controlID = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
            conf._mDName = "WhiteBalance";
            conf._values.push_back(2800);
            conf._values.push_back(3540);
            conf._values.push_back(4280);
            conf._values.push_back(5020);
            conf._values.push_back(5760);
            conf._values.push_back(6500);
        }
        return conf;
    }
};