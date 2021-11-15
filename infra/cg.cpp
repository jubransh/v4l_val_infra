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
            conf._values.push_back(10);
            conf._values.push_back(155);
            conf._values.push_back(322);
            conf._values.push_back(655);
            conf._values.push_back(1650);            
        }
        else if (controlName == "Gain")
        {
            conf._controlName = "Gain";
            conf._controlID = V4L2_CID_GAIN;
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
        return conf;
    }
};