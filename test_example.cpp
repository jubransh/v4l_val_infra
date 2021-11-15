
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
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <thread>
#include "infra/CameraInfra.cpp"

class TestBase : public testing::Test
{

};


class TestConfiguration
{
    int duration;
    int itertions;
    
};


class StreamingTest : public TestBase
{

protected:
    void SetUp() override
    {        
        string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        cout << "=========================================\n";
        cout << "init logs for test " << testName << endl;
    }

    void TearDown() override
    {
    }

    void RunFlow(TestConfiguration tC)
    {
    }
};


TEST_F(StreamingTest, DepthStreamingExample)
{
    TestConfiguration tc;
    RunFlow(tc);
}

TEST_F(StreamingTest, IRStreamingExample)
{
    TestConfiguration tc;
    RunFlow(tc);
}

TEST_F(StreamingTest, ColorStreamingExample)
{
    TestConfiguration tc;
    RunFlow(tc);
}

TEST_F(StreamingTest, DepthColorStreamingExample)
{
    TestConfiguration tc;
    RunFlow(tc);
}