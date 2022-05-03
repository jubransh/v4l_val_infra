#include <chrono>
#include <thread>
#include <mutex>
#include "tbb/concurrent_queue.h"
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc.hpp"

#include <sys/types.h>
#include <pwd.h>

using namespace std;
using namespace cv;

class File_Utils
{
private:
public:
    static string getHomeDir()
    {

        struct passwd* pw = getpwuid(getuid());

        char* homedir = pw->pw_dir;
        return homedir;
    }
    static bool isDirExist(const std::string &path)
    {
#if defined(_WIN32)
        struct _stat info;
        if (_stat(path.c_str(), &info) != 0)
        {
            return false;
        }
        return (info.st_mode & _S_IFDIR) != 0;
#else
        struct stat info;
        if (stat(path.c_str(), &info) != 0)
        {
            return false;
        }
        return (info.st_mode & S_IFDIR) != 0;
#endif
    }

    static string join(string base, string subPath)
    {
#if defined(_WIN32)
        string fullPath = base + "\\" + subPath;
        return fullPath;
#else
        string fullPath = base + "/" + subPath;
        return fullPath;
#endif
    }

    static bool makePath(const std::string &path)
    {
        if (!isDirExist(path))
        {
#if defined(_WIN32)
            int ret = _mkdir(path.c_str());
#else
            mode_t mode = 0755;
            int ret = mkdir(path.c_str(), mode);
#endif
            if (ret == 0)
                return true;

            switch (errno)
            {
            case ENOENT:
                // parent didn't exist, try to create it
                {
                    int pos = path.find_last_of('/');
                    if (pos == std::string::npos)
#if defined(_WIN32)
                        pos = path.find_last_of('\\');
                    if (pos == std::string::npos)
#endif
                        return false;
                    if (!makePath(path.substr(0, pos)))
                        return false;
                }
                // now, try to create again
#if defined(_WIN32)
                return 0 == _mkdir(path.c_str());
#else
                return 0 == mkdir(path.c_str(), mode);
#endif

            case EEXIST:
                // done!
                return isDirExist(path);

            default:
                return false;
            }
        }
        else
            Logger::getLogger().log("directory already exists", "FrameAnalyzer", LOG_INFO);
        return true;
    }
};

struct CorruptedResult
{
    int width;
    int height;
    string pixelFormat;
    int fps;
    int frame_id;
    double std;
    double mean;
    bool is_corrupted;
    bool is_high_or_low_image_pixels;
};

struct FreezeResult
{
    int width;
    int height;
    string pixelFormat;
    int fps;
    int first_frame_id;
    int second_frame_id;
    bool is_freeze;
};

struct AnalayzerFrame
{
    Frame frame;
    int width;
    int height;
    int size;
    string pixelFormat;
    int fps;
};

void write_image_to_bin_file(string filePath, uint8_t *ptr, size_t len)
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
        Logger::getLogger().log("Failed to write image to bin file", "FrameAnalyzer", LOG_ERROR);
    }
}

class FrameAnalyzer
{
    tbb::concurrent_bounded_queue<AnalayzerFrame> _depth_queue;
    std::vector<CorruptedResult> _depth_corrupted_results;
    std::vector<FreezeResult> _depth_freeze_results;
    int _depth_first_corrpted_index = -1;
    int _depth_first_freeze_index = -1;

    tbb::concurrent_bounded_queue<AnalayzerFrame> _color_queue;
    std::vector<CorruptedResult> _color_corrupted_results;
    std::vector<FreezeResult> _color_freeze_results;
    int _color_first_corrpted_index = -1;
    int _color_first_freeze_index = -1;

    tbb::concurrent_bounded_queue<AnalayzerFrame> _infrared_queue;
    std::vector<CorruptedResult> _infrared_corrupted_results;
    std::vector<FreezeResult> _infrared_freeze_results;
    int _infrared_first_corrpted_index = -1;
    int _infrared_first_freeze_index = -1;

    std::vector<std::thread> _collect_threads;
    std::mutex _stop_mutex;
    int _save_image_count;
    int _first_frames_to_skip;
    bool _stop_collecting;
    string _csv_root_path;
    ofstream _corrupted_res_csv;
    ofstream _freeze_res_csv;
    int _iteration;

    void init_all()
    {
        _collect_threads.clear();
        _stop_collecting = false;

        // init depth variables
        _depth_queue.clear();
        _depth_corrupted_results.clear();
        _depth_freeze_results.clear();
        _depth_first_corrpted_index = -1;
        _depth_first_freeze_index = -1;

        // init color variables
        _color_queue.clear();
        _color_corrupted_results.clear();
        _color_freeze_results.clear();
        _color_first_corrpted_index = -1;
        _color_first_freeze_index = -1;

        // init infrared variables
        _infrared_queue.clear();
        _infrared_corrupted_results.clear();
        _infrared_freeze_results.clear();
        _infrared_first_corrpted_index = -1;
        _infrared_first_freeze_index = -1;
    }

    int get_std_tolerance(StreamType stream_type)
    {
        if (stream_type == StreamType::Depth_Stream)
            return 50;
        else if (stream_type == StreamType::IR_Stream)
            return 60;
        else
            return 30;
    }

    int get_max_image_mean(StreamType stream_type)
    {
        if (stream_type == StreamType::Depth_Stream)
            return 600;
        else if (stream_type == StreamType::IR_Stream)
            return 250;
        else
            return 250;
    }

    int get_min_image_mean(StreamType stream_type)
    {
        if (stream_type == StreamType::Depth_Stream)
            return 400;
        else if (stream_type == StreamType::IR_Stream)
            return 50;
        else
            return 50;
    }

    double to_radians(double degree)
    {
        return (degree * M_PI / 180);
    }

    void *CropImageArray(uint8_t *outputPixels, uint8_t *pixels, int sourceWidth, int sourceHeight, int pixelsToCut)
    {
        int i = 0;
        int j = 0;
        for (int col = pixelsToCut * 2; col < sourceWidth * sourceHeight * 2 - 1; col += sourceWidth * 2)
        {
            for (int j = col; j < col + (sourceWidth - pixelsToCut) * 2; j++)
            {
                outputPixels[i] = pixels[j];
                i++;
            }
        }
    }

    FreezeResult calc_freeze_frames(AnalayzerFrame frame1, AnalayzerFrame frame2)
    {
        FreezeResult fr;
        int is_equal;
        is_equal = memcmp(frame1.frame.Buff, frame2.frame.Buff, frame1.frame.buffSize);
        fr.fps = frame1.fps;
        fr.width = frame1.width;
        fr.height = frame1.height;
        fr.pixelFormat = frame1.pixelFormat;
        fr.first_frame_id = frame1.frame.ID;
        fr.second_frame_id = frame2.frame.ID;
        if (is_equal == 0)
        {
            fr.is_freeze = true;

            if (frame1.pixelFormat == "UYVY" || frame1.pixelFormat == "Y8"|| frame1.pixelFormat == "Y8i")
            {
                if (_infrared_first_freeze_index == -1)
                    _infrared_first_freeze_index = frame1.frame.ID;
            }
            else if (frame1.pixelFormat == "YUYV")
            {
                if (_color_first_freeze_index == -1)
                    _color_first_freeze_index = frame1.frame.ID;
            }
            else if (frame1.pixelFormat == "Z16")
            {
                if (_depth_first_freeze_index == -1)
                    _depth_first_freeze_index = frame1.frame.ID;
            }
        }
        else
        {
            fr.is_freeze = false;
        }
        return fr;
    }

    CorruptedResult calc_corrupted_frame(AnalayzerFrame frame)
    {
        int width = frame.width;
        int height = frame.height;
        CorruptedResult cr;
        cv::Mat map;
        cv::Mat rgbMap;
        cv::Mat grayScaleMap;
        Scalar mean, stddev;
        if (frame.pixelFormat == "Z16")
        {
            // Cut dead zone
            int baseline = 95;
            int hfov[] = {72, 84};
            int target_distance = 500;
            int selected_hfov;
            double hfov_rad;
            double percent_to_cut;
            int pixels_to_cut;
            if (abs(width - 640) < abs(width - 1280))
            {
                selected_hfov = hfov[0];
            }
            else
            {
                selected_hfov = hfov[1];
            }
            hfov_rad = to_radians(selected_hfov) / 2;
            percent_to_cut = baseline / (2 * target_distance * tan(hfov_rad));
            pixels_to_cut = ceil(percent_to_cut * width);
            uint8_t outputPixels[(width - pixels_to_cut) * height * 2];
            CropImageArray((uint8_t *)outputPixels, (uint8_t *)frame.frame.Buff, width, height, pixels_to_cut);
            map = cv::Mat(height, width - pixels_to_cut, CV_16UC1, (uint8_t *)outputPixels);
            // calculate image std
            cv::meanStdDev(map, mean, stddev);
            cr.fps = frame.fps;
            cr.width = frame.width;
            cr.height = frame.height;
            cr.pixelFormat = frame.pixelFormat;
            cr.frame_id = frame.frame.ID;
            cr.std = stddev[0];
            cr.mean = mean[0];

            if (stddev[0] > get_std_tolerance(StreamType::Depth_Stream))
            {
                cr.is_corrupted = true;
                if (_depth_first_corrpted_index == -1)
                    _depth_first_corrpted_index = frame.frame.ID;
            }
            else
                cr.is_corrupted = false;
            if (mean[0] < get_min_image_mean(StreamType::Depth_Stream) || mean[0] > get_max_image_mean(StreamType::Depth_Stream))
            {
                cr.is_corrupted = true;
                cr.is_high_or_low_image_pixels = true;
                if (_depth_first_corrpted_index == -1)
                    _depth_first_corrpted_index = frame.frame.ID;
            }
            else
            {
                cr.is_high_or_low_image_pixels = false;
            }
        }
        else if (frame.pixelFormat == "YUYV")
        {
            map = cv::Mat(height, width, CV_8UC2, (uint8_t *)frame.frame.Buff);
            cv::cvtColor(map, rgbMap, COLOR_YUV2RGB_YUYV);
            cv::cvtColor(rgbMap, grayScaleMap, COLOR_RGB2GRAY);
            cv::meanStdDev(grayScaleMap, mean, stddev);
            cr.fps = frame.fps;
            cr.width = frame.width;
            cr.height = frame.height;
            cr.pixelFormat = frame.pixelFormat;
            cr.frame_id = frame.frame.ID;
            cr.std = stddev[0];
            cr.mean = mean[0];
            if (stddev[0] > get_std_tolerance(StreamType::Color_Stream))
            {
                cr.is_corrupted = true;
                if (_color_first_corrpted_index == -1)
                    _color_first_corrpted_index = frame.frame.ID;
            }
            else
                cr.is_corrupted = false;
            if (mean[0] < get_min_image_mean(StreamType::Color_Stream) || mean[0] > get_max_image_mean(StreamType::Color_Stream))
            {
                cr.is_corrupted = true;
                cr.is_high_or_low_image_pixels = true;
                if (_color_first_corrpted_index == -1)
                    _color_first_corrpted_index = frame.frame.ID;
            }
            else
            {
                cr.is_high_or_low_image_pixels = false;
            }
        }
        else if (frame.pixelFormat == "Y8" || frame.pixelFormat == "Y8i")
        {
            map = cv::Mat(height, width, CV_8UC1, (uint8_t *)frame.frame.Buff);
            cv::cvtColor(map, rgbMap, COLOR_GRAY2RGB);
            cv::cvtColor(rgbMap, grayScaleMap, COLOR_RGB2GRAY);
            cv::meanStdDev(grayScaleMap, mean, stddev);
            cr.fps = frame.fps;
            cr.width = frame.width;
            cr.height = frame.height;
            cr.pixelFormat = frame.pixelFormat;
            cr.frame_id = frame.frame.ID;
            cr.std = stddev[0];
            cr.mean = mean[0];
            if (stddev[0] > get_std_tolerance(StreamType::IR_Stream))
            {
                cr.is_corrupted = true;
                if (_infrared_first_corrpted_index == -1)
                    _infrared_first_corrpted_index = frame.frame.ID;
            }
            else
                cr.is_corrupted = false;
            if (mean[0] < get_min_image_mean(StreamType::IR_Stream) || mean[0] > get_max_image_mean(StreamType::IR_Stream))
            {
                cr.is_corrupted = true;
                cr.is_high_or_low_image_pixels = true;
                if (_infrared_first_corrpted_index == -1)
                    _infrared_first_corrpted_index = frame.frame.ID;
            }
            else
            {
                cr.is_high_or_low_image_pixels = false;
            }
        }
        else if (frame.pixelFormat == "UYVY")
        {
            map = cv::Mat(height, width, CV_8UC2, (uint8_t *)frame.frame.Buff);
            cv::cvtColor(map, rgbMap, COLOR_YUV2RGB_UYVY);
            cv::cvtColor(rgbMap, grayScaleMap, COLOR_RGB2GRAY);
            cv::meanStdDev(grayScaleMap, mean, stddev);
            cr.fps = frame.fps;
            cr.width = frame.width;
            cr.height = frame.height;
            cr.pixelFormat = frame.pixelFormat;
            cr.frame_id = frame.frame.ID;
            cr.std = stddev[0];
            cr.mean = mean[0];
            if (stddev[0] > get_std_tolerance(StreamType::IR_Stream))
            {
                cr.is_corrupted = true;
                if (_infrared_first_corrpted_index == -1)
                    _infrared_first_corrpted_index = frame.frame.ID;
            }
            else
                cr.is_corrupted = false;
            if (mean[0] < get_min_image_mean(StreamType::IR_Stream) || mean[0] > get_max_image_mean(StreamType::IR_Stream))
            {
                cr.is_corrupted = true;
                cr.is_high_or_low_image_pixels = true;
                if (_infrared_first_corrpted_index == -1)
                    _infrared_first_corrpted_index = frame.frame.ID;
            }
            else
            {
                cr.is_high_or_low_image_pixels = false;
            }
        }
        return cr;
    }

    void analyze_depth_frames()
    {
        int saved_corrupted_frames = 0;
        int saved_freeze_frames = 0;
        AnalayzerFrame first_frame;
        AnalayzerFrame second_frame;
        bool is_first_frame = true;
        while (1)
        {
            try
            {
                AnalayzerFrame frame;
                _depth_queue.pop(frame);
                if (frame.frame.ID <= _first_frames_to_skip)
                {
                    continue;
                }
                // cout << "FrameAnalyzer: analyzing depth frame: " << frame.frame.ID << endl;
                CorruptedResult cr = calc_corrupted_frame(frame);
                _depth_corrupted_results.push_back(cr);
                if (cr.is_corrupted)
                {
                    if (saved_corrupted_frames < _save_image_count)
                    {
                        string image_name = "depth_" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(frame.frame.ID) + ".bin";
                        string image_path = File_Utils::join(_csv_root_path, "corrupted_frames");
                        image_path = File_Utils::join(image_path, to_string(_iteration));
                        if (!File_Utils::isDirExist(image_path))
                            File_Utils::makePath(image_path);
                        image_path = File_Utils::join(image_path, image_name);
                        write_image_to_bin_file(image_path, frame.frame.Buff, frame.frame.buffSize);
                        saved_corrupted_frames++;
                    }
                }
                if (is_first_frame)
                {
                    first_frame = frame;
                    first_frame.frame.Buff = (uint8_t *)malloc(frame.frame.buffSize);
                    memcpy(first_frame.frame.Buff, frame.frame.Buff, frame.frame.buffSize);
                    is_first_frame = false;
                }
                else
                {
                    second_frame = frame;
                    second_frame.frame.Buff = (uint8_t *)malloc(frame.frame.buffSize);
                    memcpy(second_frame.frame.Buff, frame.frame.Buff, frame.frame.buffSize);
                    FreezeResult fr = calc_freeze_frames(first_frame, second_frame);
                    _depth_freeze_results.push_back(fr);
                    if (fr.is_freeze)
                    {
                        if (saved_freeze_frames < _save_image_count)
                        {
                            string image1_name = "depth_first_" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(first_frame.frame.ID) + ".bin";
                            string image2_name = "depth_second" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(second_frame.frame.ID) + ".bin";
                            string image_path = File_Utils::join(_csv_root_path, "freeze_frames");
                            image_path = File_Utils::join(image_path, to_string(_iteration));
                            if (!File_Utils::isDirExist(image_path))
                                File_Utils::makePath(image_path);
                            string image1_path = File_Utils::join(image_path, image1_name);
                            string image2_path = File_Utils::join(image_path, image2_name);
                            write_image_to_bin_file(image1_path, first_frame.frame.Buff, frame.frame.buffSize);
                            write_image_to_bin_file(image2_path, second_frame.frame.Buff, frame.frame.buffSize);
                            saved_freeze_frames++;
                        }
                    }
                    free(first_frame.frame.Buff);
                    first_frame = second_frame;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
            if (_stop_collecting)
            {
                if (_depth_freeze_results.size() > 0)
                    free(second_frame.frame.Buff);
                Logger::getLogger().log("Collecting depth frames stopped", "FrameAnalyzer", LOG_INFO);
                break;
            }
        }
    }

    void analyze_color_frames()
    {
        int saved_corrupted_frames = 0;
        int saved_freeze_frames = 0;
        AnalayzerFrame first_frame;
        AnalayzerFrame second_frame;
        bool is_first_frame = true;
        while (1)
        {

            try
            {
                AnalayzerFrame frame;
                _color_queue.pop(frame);
                if (frame.frame.ID <= _first_frames_to_skip)
                {
                    continue;
                }
                // cout << "FrameAnalyzer: analyzing color frame: " << frame.frame.ID << endl;
                CorruptedResult cr = calc_corrupted_frame(frame);
                _color_corrupted_results.push_back(cr);
                if (cr.is_corrupted)
                {
                    if (saved_corrupted_frames < _save_image_count)
                    {
                        string image_name = "color_" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(frame.frame.ID) + ".bin";
                        string image_path = File_Utils::join(_csv_root_path, "corrupted_frames");
                        image_path = File_Utils::join(image_path, to_string(_iteration));
                        if (!File_Utils::isDirExist(image_path))
                            File_Utils::makePath(image_path);
                        image_path = File_Utils::join(image_path, image_name);
                        write_image_to_bin_file(image_path, frame.frame.Buff, frame.frame.buffSize);
                        saved_corrupted_frames++;
                    }
                }
                if (is_first_frame)
                {
                    first_frame = frame;
                    first_frame.frame.Buff = (uint8_t *)malloc(frame.frame.buffSize);
                    memcpy(first_frame.frame.Buff, frame.frame.Buff, frame.frame.buffSize);
                    is_first_frame = false;
                }
                else
                {
                    second_frame = frame;
                    second_frame.frame.Buff = (uint8_t *)malloc(frame.frame.buffSize);
                    memcpy(second_frame.frame.Buff, frame.frame.Buff, frame.frame.buffSize);
                    FreezeResult fr = calc_freeze_frames(first_frame, second_frame);
                    _color_freeze_results.push_back(fr);
                    if (fr.is_freeze)
                    {
                        if (saved_freeze_frames < _save_image_count)
                        {
                            string image1_name = "color_first_" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(first_frame.frame.ID) + ".bin";
                            string image2_name = "color_second" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(second_frame.frame.ID) + ".bin";
                            string image_path = File_Utils::join(_csv_root_path, "freeze_frames");
                            image_path = File_Utils::join(image_path, to_string(_iteration));
                            if (!File_Utils::isDirExist(image_path))
                                File_Utils::makePath(image_path);
                            string image1_path = File_Utils::join(image_path, image1_name);
                            string image2_path = File_Utils::join(image_path, image2_name);
                            write_image_to_bin_file(image1_path, first_frame.frame.Buff, frame.frame.buffSize);
                            write_image_to_bin_file(image2_path, second_frame.frame.Buff, frame.frame.buffSize);
                            saved_freeze_frames++;
                        }
                    }
                    free(first_frame.frame.Buff);
                    first_frame = second_frame;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
            if (_stop_collecting)
            {
                if (_color_freeze_results.size() > 0)
                    free(second_frame.frame.Buff);
                Logger::getLogger().log("Collecting color frames stopped", "FrameAnalyzer", LOG_INFO);
                break;
            }
        }
    }

    void analyze_infrared_frames()
    {
        int saved_corrupted_frames = 0;
        int saved_freeze_frames = 0;
        AnalayzerFrame first_frame;
        AnalayzerFrame second_frame;
        bool is_first_frame = true;
        while (1)
        {

            try
            {
                AnalayzerFrame frame;
                _infrared_queue.pop(frame);
                if (frame.frame.ID <= _first_frames_to_skip)
                {
                    continue;
                }
                // cout << "FrameAnalyzer: analyzing infrared frame: " << frame.frame.ID << endl;
                CorruptedResult cr = calc_corrupted_frame(frame);
                _infrared_corrupted_results.push_back(cr);
                if (cr.is_corrupted)
                {
                    if (saved_corrupted_frames < _save_image_count)
                    {
                        string image_name = "infrared_" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(frame.frame.ID) + ".bin";
                        string image_path = File_Utils::join(_csv_root_path, "corrupted_frames");
                        image_path = File_Utils::join(image_path, to_string(_iteration));
                        if (!File_Utils::isDirExist(image_path))
                            File_Utils::makePath(image_path);
                        image_path = File_Utils::join(image_path, image_name);
                        write_image_to_bin_file(image_path, frame.frame.Buff, frame.frame.buffSize);
                        saved_corrupted_frames++;
                    }
                }
                if (is_first_frame)
                {
                    first_frame = frame;
                    first_frame.frame.Buff = (uint8_t *)malloc(frame.frame.buffSize);
                    memcpy(first_frame.frame.Buff, frame.frame.Buff, frame.frame.buffSize);
                    is_first_frame = false;
                }
                else
                {
                    second_frame = frame;
                    second_frame.frame.Buff = (uint8_t *)malloc(frame.frame.buffSize);
                    memcpy(second_frame.frame.Buff, frame.frame.Buff, frame.frame.buffSize);
                    FreezeResult fr = calc_freeze_frames(first_frame, second_frame);
                    _infrared_freeze_results.push_back(fr);
                    if (fr.is_freeze)
                    {
                        if (saved_freeze_frames < _save_image_count)
                        {
                            string image1_name = "infrared_first_" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(first_frame.frame.ID) + ".bin";
                            string image2_name = "infrared_second" + to_string(frame.width) + "x" + to_string(frame.height) + "_" + to_string(second_frame.frame.ID) + ".bin";
                            string image_path = File_Utils::join(_csv_root_path, "freeze_frames");
                            image_path = File_Utils::join(image_path, to_string(_iteration));
                            if (!File_Utils::isDirExist(image_path))
                                File_Utils::makePath(image_path);
                            string image1_path = File_Utils::join(image_path, image1_name);
                            string image2_path = File_Utils::join(image_path, image2_name);
                            write_image_to_bin_file(image1_path, first_frame.frame.Buff, frame.frame.buffSize);
                            write_image_to_bin_file(image2_path, second_frame.frame.Buff, frame.frame.buffSize);
                            saved_freeze_frames++;
                        }
                    }
                    free(first_frame.frame.Buff);
                    first_frame = second_frame;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
            if (_stop_collecting)
            {
                if (_infrared_freeze_results.size() > 0)
                    free(second_frame.frame.Buff);
                Logger::getLogger().log("Collecting infrared frames stopped", "FrameAnalyzer", LOG_INFO);
                break;
            }
        }
    }

    void save_corrupted_results()
    {
        string depth_iteration_summary = "";
        string infrared_iteration_summary = "";
        string color_iteration_summary = "";
        Logger::getLogger().log("Saving corrupted results to the CSV file", "FrameAnalyzer", LOG_INFO);
        string corrupted_path = File_Utils::join(_csv_root_path, "corrupted_results.csv");
        _corrupted_res_csv.open(corrupted_path, std::ios_base::app);
        if (_corrupted_res_csv.fail())
        {
            Logger::getLogger().log("Cannot open corrupted results file" + corrupted_path, "FrameAnalyzer", LOG_ERROR);
            throw std::runtime_error("Cannot open file: " + corrupted_path);
        }
        if (_iteration == 0)
            _corrupted_res_csv << "stream_type,resolution,fps,format,index,result_type,iteration_id,total_frames_analyzed,is_corrupted_frame,frame_pixels_std,corrupted_frames_count,is_high_or_low_frame_pixels,frame_pixels_mean,high_or_low_frames_count,first_corrupted_frame_index" << endl;
        // saving depth frames results
        if (_depth_corrupted_results.size() > 0)
        {
            int corrupted_count = 0;
            int high_or_low_images_count = 0;
            string status = "false";
            string high_low_status = "false";
            for (int i = 0; i < _depth_corrupted_results.size(); i++)
            {
                status = "false";
                high_low_status = "false";
                string res = to_string(_depth_corrupted_results[i].width) + "x" + to_string(_depth_corrupted_results[i].height);
                if (_depth_corrupted_results[i].is_corrupted)
                {
                    status = "true";
                }
                if (_depth_corrupted_results[i].is_high_or_low_image_pixels)
                {
                    status = "true";
                    high_low_status = "true";
                    high_or_low_images_count++;
                }
                if (status == "true")
                {
                    corrupted_count++;
                }
                _corrupted_res_csv << "depth," << res << "," << _depth_corrupted_results[i].fps << "," << _depth_corrupted_results[i].pixelFormat << "," << _depth_corrupted_results[i].frame_id << ","
                                   << "single," << _iteration << ",," << status << "," << _depth_corrupted_results[i].std << ",," << high_low_status << "," << _depth_corrupted_results[i].mean << ",,," << endl;
            }
            depth_iteration_summary = "depth,,,,,iteration," + to_string(_iteration) + "," + to_string(_depth_corrupted_results.size()) + ",,," + to_string(corrupted_count) + ",,," + to_string(high_or_low_images_count) + "," + to_string(_depth_first_corrpted_index) + "\n";
        }

        // saving infrared frames results
        if (_infrared_corrupted_results.size() > 0)
        {
            int corrupted_count = 0;
            int high_or_low_images_count = 0;
            string status;
            string high_low_status;
            for (int i = 0; i < _infrared_corrupted_results.size(); i++)
            {
                status = "false";
                high_low_status = "false";
                string res = to_string(_infrared_corrupted_results[i].width) + "x" + to_string(_infrared_corrupted_results[i].height);
                if (_infrared_corrupted_results[i].is_corrupted)
                {
                    status = "true";
                }
                if (_infrared_corrupted_results[i].is_high_or_low_image_pixels)
                {
                    status = "true";
                    high_low_status = "true";
                    high_or_low_images_count++;
                }
                if (status == "true")
                {
                    corrupted_count++;
                }
                _corrupted_res_csv << "infrared," << res << "," << _infrared_corrupted_results[i].fps << "," << _infrared_corrupted_results[i].pixelFormat << "," << _infrared_corrupted_results[i].frame_id << ","
                                   << "single," << _iteration << ",," << status << "," << _infrared_corrupted_results[i].std << ",," << high_low_status << "," << _infrared_corrupted_results[i].mean << ",,," << endl;
            }
            infrared_iteration_summary = "infrared,,,,,iteration," + to_string(_iteration) + "," + to_string(_infrared_corrupted_results.size()) + ",,," + to_string(corrupted_count) + ",,," + to_string(high_or_low_images_count) + "," + to_string(_infrared_first_corrpted_index) + "\n";
        }

        // saving color frames results
        if (_color_corrupted_results.size() > 0)
        {
            int corrupted_count = 0;
            int high_or_low_images_count = 0;
            string status;
            string high_low_status;
            for (int i = 0; i < _color_corrupted_results.size(); i++)
            {
                status = "false";
                high_low_status = "false";
                string res = to_string(_color_corrupted_results[i].width) + "x" + to_string(_color_corrupted_results[i].height);
                if (_color_corrupted_results[i].is_corrupted)
                {
                    status = "true";
                }
                if (_color_corrupted_results[i].is_high_or_low_image_pixels)
                {
                    status = "true";
                    high_low_status = "true";
                    high_or_low_images_count++;
                }
                if (status == "true")
                {
                    corrupted_count++;
                }
                _corrupted_res_csv << "color," << res << "," << _color_corrupted_results[i].fps << "," << _color_corrupted_results[i].pixelFormat << "," << _color_corrupted_results[i].frame_id << ","
                                   << "single," << _iteration << ",," << status << "," << _color_corrupted_results[i].std << ",," << high_low_status << "," << _color_corrupted_results[i].mean << ",,," << endl;
            }
            color_iteration_summary = "color,,,,,iteration," + to_string(_iteration) + "," + to_string(_color_corrupted_results.size()) + ",,," + to_string(corrupted_count) + ",,," + to_string(high_or_low_images_count) + "," + to_string(_color_first_corrpted_index) + "\n";
        }
        _corrupted_res_csv << depth_iteration_summary;
        _corrupted_res_csv << infrared_iteration_summary;
        _corrupted_res_csv << color_iteration_summary;

        _corrupted_res_csv.close();
    }

    void save_freeze_results()
    {
        string depth_iteration_summary = "";
        string infrared_iteration_summary = "";
        string color_iteration_summary = "";
        Logger::getLogger().log("Saving freeze results to the CSV file", "FrameAnalyzer", LOG_INFO);
        string freeze_path = File_Utils::join(_csv_root_path, "freeze_results.csv");
        _freeze_res_csv.open(freeze_path, std::ios_base::app);
        if (_freeze_res_csv.fail())
        {
            Logger::getLogger().log("Cannot open freeze results file: " + freeze_path, "FrameAnalyzer", LOG_ERROR);
            throw std::runtime_error("Cannot open file: " + freeze_path);
        }
        if (_iteration == 0)
            _freeze_res_csv << "stream_type,resolution,fps,format,indexes,result_type,iteration_id,total_frames_analyzed,is_freeze,freezes_count,first_freeze_index,max_sequential_freeze, max_sequential_freeze_index" << endl;
        // saving depth frames results
        if (_depth_freeze_results.size() > 0)
        {
            int freeze_count = 0;
            int first_seq_index = -1;
            int last_seq_index = -1;
            int max_sequential = 0;
            int max_sequential_index = -1;
            for (int i = 0; i < _depth_freeze_results.size(); i++)
            {
                string status;
                FreezeResult fr = _depth_freeze_results[i];
                string res = to_string(fr.width) + "x" + to_string(fr.height);
                string indexes = to_string(fr.first_frame_id) + "_" + to_string(fr.second_frame_id);
                if (fr.is_freeze)
                {
                    if (first_seq_index == -1)
                    {
                        first_seq_index = fr.first_frame_id;
                    }
                    last_seq_index = fr.second_frame_id;
                    if (last_seq_index - first_seq_index > max_sequential)
                    {
                        max_sequential = last_seq_index - first_seq_index;
                        max_sequential_index = first_seq_index;
                    }
                    status = "true";
                    freeze_count++;
                }
                else
                {
                    first_seq_index = -1;
                    last_seq_index = -1;
                    status = "false";
                }
                _freeze_res_csv << "depth," << res << "," << fr.fps << "," << fr.pixelFormat << "," << indexes << ","
                                << "single," << _iteration << ",," << status << ",,,," << endl;
            }
            depth_iteration_summary = "depth,,,,,iteration," + to_string(_iteration) + "," + to_string(_depth_freeze_results.size()) + ",," + to_string(freeze_count) + "," + to_string(_depth_first_freeze_index) + "," + to_string(max_sequential) + "," + to_string(max_sequential_index) + "\n";
        }

        // saving infrared frames results
        if (_infrared_freeze_results.size() > 0)
        {
            int freeze_count = 0;
            int first_seq_index = -1;
            int last_seq_index = -1;
            int max_sequential = 0;
            int max_sequential_index = -1;
            for (int i = 0; i < _infrared_freeze_results.size(); i++)
            {
                string status;
                FreezeResult fr = _infrared_freeze_results[i];
                string res = to_string(fr.width) + "x" + to_string(fr.height);
                string indexes = to_string(fr.first_frame_id) + "_" + to_string(fr.second_frame_id);
                if (fr.is_freeze)
                {
                    if (first_seq_index == -1)
                    {
                        first_seq_index = fr.first_frame_id;
                    }
                    last_seq_index = fr.second_frame_id;
                    if (last_seq_index - first_seq_index > max_sequential)
                    {
                        max_sequential = last_seq_index - first_seq_index;
                        max_sequential_index = first_seq_index;
                    }
                    status = "true";
                    freeze_count++;
                }
                else
                {
                    first_seq_index = -1;
                    last_seq_index = -1;
                    status = "false";
                }
                _freeze_res_csv << "infrared," << res << "," << fr.fps << "," << fr.pixelFormat << "," << indexes << ","
                                << "single," << _iteration << ",," << status << ",,,," << endl;
            }
            infrared_iteration_summary = "infrared,,,,,iteration," + to_string(_iteration) + "," + to_string(_infrared_freeze_results.size()) + ",," + to_string(freeze_count) + "," + to_string(_infrared_first_freeze_index) + "," + to_string(max_sequential) + "," + to_string(max_sequential_index) + "\n";
        }

        // saving color frames results
        if (_color_freeze_results.size() > 0)
        {
            int freeze_count = 0;
            int first_seq_index = -1;
            int last_seq_index = -1;
            int max_sequential = 0;
            int max_sequential_index = -1;
            for (int i = 0; i < _color_freeze_results.size(); i++)
            {
                string status;
                FreezeResult fr = _color_freeze_results[i];
                string res = to_string(fr.width) + "x" + to_string(fr.height);
                string indexes = to_string(fr.first_frame_id) + "_" + to_string(fr.second_frame_id);
                if (fr.is_freeze)
                {
                    if (first_seq_index == -1)
                    {
                        first_seq_index = fr.first_frame_id;
                    }
                    last_seq_index = fr.second_frame_id;
                    if (last_seq_index - first_seq_index > max_sequential)
                    {
                        max_sequential = last_seq_index - first_seq_index;
                        max_sequential_index = first_seq_index;
                    }
                    status = "true";
                    freeze_count++;
                }
                else
                {
                    first_seq_index = -1;
                    last_seq_index = -1;
                    status = "false";
                }
                _freeze_res_csv << "color," << res << "," << fr.fps << "," << fr.pixelFormat << "," << indexes << ","
                                << "single," << _iteration << ",," << status << ",,,," << endl;
            }
            color_iteration_summary = "color,,,,,iteration," + to_string(_iteration) + "," + to_string(_color_freeze_results.size()) + ",," + to_string(freeze_count) + "," + to_string(_color_first_freeze_index) + "," + to_string(max_sequential) + "," + to_string(max_sequential_index) + "\n";
        }

        _freeze_res_csv << depth_iteration_summary;
        _freeze_res_csv << infrared_iteration_summary;
        _freeze_res_csv << color_iteration_summary;

        _freeze_res_csv.close();
    }

public:
    FrameAnalyzer()
    {
        _stop_collecting = false;
        _save_image_count = 10;
        _first_frames_to_skip = 15;
        _csv_root_path = File_Utils::getHomeDir() + "/Logs";
        _depth_queue.set_capacity(1);
        _color_queue.set_capacity(1);
        _infrared_queue.set_capacity(1);
        _iteration = -1;
    }

    void configure(string csv_root_path, int images_to_save_count, int first_frames_to_skip)
    {
        _save_image_count = images_to_save_count;
        _first_frames_to_skip = first_frames_to_skip;
        _csv_root_path = csv_root_path;
    }

    bool collect_depth_frame(AnalayzerFrame frame)
    {
        return _depth_queue.try_push(frame);
    }

    bool collect_color_frame(AnalayzerFrame frame)
    {
        return _color_queue.try_push(frame);
    }

    bool collect_infrared_frame(AnalayzerFrame frame)
    {
        return _infrared_queue.try_push(frame);
    }

    void start_collection()
    {
        init_all();
        _iteration++;
        Logger::getLogger().log("Start collecting", "FrameAnalyzer", LOG_INFO);
        _collect_threads.push_back(std::thread(&FrameAnalyzer::analyze_depth_frames, this));
        _collect_threads.push_back(std::thread(&FrameAnalyzer::analyze_color_frames, this));
        _collect_threads.push_back(std::thread(&FrameAnalyzer::analyze_infrared_frames, this));
    }

    void stop_collection()
    {
        Logger::getLogger().log("Stop collecting", "FrameAnalyzer", LOG_INFO);
        _stop_collecting = true;
        _depth_queue.abort();
        _color_queue.abort();
        _infrared_queue.abort();

        for (int i = 0; i < _collect_threads.size(); i++)
        {
            _collect_threads[i].join();
        }
    }

    void reset()
    {
        _iteration = -1;
    }

    void save_results()
    {
        save_corrupted_results();
        save_freeze_results();
    }

    vector<CorruptedResult> get_depth_corrupted_results()
    {
        return _depth_corrupted_results;
    }
    vector<CorruptedResult> get_color_corrupted_results()
    {
        return _color_corrupted_results;
    }
    vector<CorruptedResult> get_infrared_corrupted_results()
    {
        return _infrared_corrupted_results;
    }

    vector<FreezeResult> get_depth_freeze_results()
    {
        return _depth_freeze_results;
    }
    vector<FreezeResult> get_color_freeze_results()
    {
        return _color_freeze_results;
    }
    vector<FreezeResult> get_infrared_freeze_results()
    {
        return _infrared_freeze_results;
    }
};