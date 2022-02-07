The Dev branch is the latest updated 

To build project:
1. Go to the project root dir
2. Run -> g++ ./main.cpp -o main -lgtest -lpthread -lopencv_core -lopencv_imgproc -ltbb


To view the list of the tests run the following command after compiling the project:
-> ./main --gtest_list_tests

To run Streaming test:
./main --gtest_filter=StreamingTest.DepthStreamingTest
./main --gtest_filter=StreamingTest.IRStreamingTest
./main --gtest_filter=StreamingTest.ColorStreamingTest
./main --gtest_filter=StreamingTest.DepthColorStreamingTest
./main --gtest_filter=StreamingTest.IRColorStreamingTest
./main --gtest_filter=StreamingTest.DepthIRColorStreamingTest

To run Stability tests:
./main --gtest_filter=StabilityTest.Normal
./main --gtest_filter=StabilityTest.Random

To run Long test:
./main --gtest_filter=LongTest.LongStreamTest
