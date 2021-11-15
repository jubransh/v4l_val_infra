/*
 * Copyright © 2019 Intel Corporation. All rights reserved.
 *
 * The source code contained or described herein and all documents related to the source code
 * ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the
 * Material remains with Intel Corporation or its suppliers and licensors. The Material may
 * contain trade secrets and proprietary and confidential information of Intel Corporation
 * and its suppliers and licensors, and is protected by worldwide copyright and trade secret
 * laws and treaty provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way
 * without Intel’s prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual property right
 * is granted to or conferred upon you by disclosure or delivery of the Materials, either
 * expressly, by implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in writing.
 */

#include <ctime>
#include <sys/mman.h>
#include "MetaData.h"

using namespace std;
#include <gtest/gtest.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <errno.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <array>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include <thread> 


using namespace std;
#define UPDC32(octet, crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

#define DS5_STREAM_CONFIG_0                  0x4000
#define DS5_CAMERA_CID_BASE                 (V4L2_CTRL_CLASS_CAMERA | DS5_STREAM_CONFIG_0)
#define DS5_CAMERA_CID_LASER_POWER          (DS5_CAMERA_CID_BASE+1)
#define DS5_CAMERA_CID_MANUAL_LASER_POWER   (DS5_CAMERA_CID_BASE+2)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_GET  (DS5_CAMERA_CID_BASE+3)
#define DS5_CAMERA_DEPTH_CALIBRATION_TABLE_SET  (DS5_CAMERA_CID_BASE+4)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_GET  (DS5_CAMERA_CID_BASE+5)
#define DS5_CAMERA_COEFF_CALIBRATION_TABLE_SET  (DS5_CAMERA_CID_BASE+6)
#define DS5_CAMERA_CID_FW_VERSION               (DS5_CAMERA_CID_BASE+7)
#define DS5_CAMERA_CID_GVD                      (DS5_CAMERA_CID_BASE+8)
#define DS5_CAMERA_CID_AE_ROI_GET               (DS5_CAMERA_CID_BASE+9)
#define DS5_CAMERA_CID_AE_ROI_SET               (DS5_CAMERA_CID_BASE+10)
#define DS5_CAMERA_CID_AE_SETPOINT_GET          (DS5_CAMERA_CID_BASE+11)
#define DS5_CAMERA_CID_AE_SETPOINT_SET          (DS5_CAMERA_CID_BASE+12)
#define DS5_CAMERA_CID_ERB                      (DS5_CAMERA_CID_BASE+13)
#define DS5_CAMERA_CID_EWB                      (DS5_CAMERA_CID_BASE+14)
#define DS5_CAMERA_CID_HWMC                     (DS5_CAMERA_CID_BASE+15)


class V4L2BasicTest : public testing::Test {
    
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

};


class Camera{
    private:
        bool stopRequested = false;
    public:
        typedef void (*FramesCallback)(string);
        FramesCallback callback;

        void stop(){
            stopRequested = true;
        }

        void start(void (*FramesCallback)(string s)){
            std::thread t1([&](){
                while(!stopRequested){
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    FramesCallback("invoked from t1");
                }
            });
            
            std::thread t2([&](){
                while(!stopRequested){
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                    FramesCallback("invoked from t2");
                }
            });

            t1.detach();
            t2.detach();

        }
};



// ================================ Main ================================

std::vector<string> frames;

//implemetnt the callback
void frameArrived(string s){
    frames.push_back(s);
    cout << s << endl;
}

TEST_F(V4L2BasicTest, main_method) {

    Camera* cam = new Camera();

    cout << " ============= Starting Stream ============= \n" ; 
    cam->start(&frameArrived);
    cout << "============= Stream Started ============= \n";
    std::this_thread::sleep_for(std::chrono::seconds(5));

    cout << "@@@@@@@@@@@@@@@@@@@@@@ Stopping Stream .....\n" ;
    cam->stop();
    cout << "@@@@@@@@@@@@@@@@@@@@@@ Streaming Stopped\n" ;

    for(int i=0; i<frames.size(); i++)
        cout << frames[i] << endl;

    ASSERT_TRUE(true);


}
