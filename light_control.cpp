using namespace std;
#include <iostream>
#include <fstream>
#include <gtest/gtest.h>

class LightSetup : public testing::Test
{

protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(LightSetup, EnableLight)
{
    cout << "=================================================" << endl;
    cout << "                 Enable light      " << endl;
    cout << "=================================================" << endl;
    string value("dimmer:255");
    //open arduino device file (linux)
    std::ofstream arduino;
    arduino.open("/dev/ttyUSB0");
    

    //write to it
    arduino << value.c_str();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    arduino.close();
}

TEST_F(LightSetup, DisableLight)
{
    cout << "=================================================" << endl;
    cout << "                 Disable light      " << endl;
    cout << "=================================================" << endl;
    string value("dimmer:0");
    //open arduino device file (linux)
    std::ofstream arduino;
    arduino.open("/dev/ttyUSB0");

    //write to it
    arduino << value.c_str();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    arduino.close();
}

TEST_F(LightSetup, SetLightValue)
{
    cout << "=================================================" << endl;
    cout << "                 Set light value      " << endl;
    cout << "=================================================" << endl;
    //0, 25, 50, 75, 100, 125, 150, 175, 200, 225, 255 
    int voltage = 125;
    string value("dimmer:" + to_string(voltage));
    //open arduino device file (linux)
    std::ofstream arduino;
    arduino.open("/dev/ttyUSB0");

    //write to it
    arduino << value.c_str();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    arduino.close();
}