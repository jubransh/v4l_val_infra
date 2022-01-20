// #include <gtest/gtest.h>

// #include "test_example.cpp"

// #include "V4L2StreamingTest.cpp"
// #include "V4L2ControlsTest.cpp"


#include <ctime>
#include <sys/mman.h>


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
#include "MetaData.h"
#include "infra/TestInfra.cpp"


#include "Streaming.cpp"
#include "Long.cpp"
// #include"Controls.cpp"
#include "Stability.cpp"

#include "content_special_tests.cpp"

// #include "V4L2PlayGround.cpp"
// #include "V4L2PlayGround2.cpp"


int main(int argc, char **argv)
{
    
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
