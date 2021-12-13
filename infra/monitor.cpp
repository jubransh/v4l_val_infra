#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <vector>
#include <thread>
#include <chrono>
 #include <fcntl.h>
 #include <sys/types.h>
 #include <sys/stat.h>

#include <fstream>
#include <numeric>
#include <unistd.h>

using namespace std;

// struct rusage {
//     struct timeval ru_utime; /* user time used */
//     struct timeval ru_stime; /* system time used */
//     long   ru_maxrss;        /* maximum resident set size */
//     long   ru_ixrss;         /* integral shared memory size */
//     long   ru_idrss;         /* integral unshared data size */
//     long   ru_isrss;         /* integral unshared stack size */
//     long   ru_minflt;        /* page reclaims */
//     long   ru_majflt;        /* page faults */
//     long   ru_nswap;         /* swaps */
//     long   ru_inblock;       /* block input operations */
//     long   ru_oublock;       /* block output operations */
//     long   ru_msgsnd;        /* messages sent */
//     long   ru_msgrcv;        /* messages received */
//     long   ru_nsignals;      /* signals received */
//     long   ru_nvcsw;         /* voluntary context switches */
//     long   ru_nivcsw;        /* involuntary context switches */
// };

const int NUM_CPU_STATES = 10;
struct Sample
    {
        int Id;
        float Cpu_per;
        int Mem_MB;
    };

class SystemMonitor
{
private:
    bool stopRequested = false;
    vector<uint64_t> v;
    std::shared_ptr<std::thread> _t;

    typedef struct CPUData
    {
        std::string cpu;
        size_t times[NUM_CPU_STATES];
    } CPUData;

std::string exec(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    std::vector<size_t> get_cpu_times() 
    {
        std::ifstream proc_stat("/proc/stat");
        proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
        std::vector<size_t> times;
        for (size_t time; proc_stat >> time; times.push_back(time));
        return times;
    }
    
    bool get_cpu_times(size_t &idle_time, size_t &total_time)
    {
        const std::vector<size_t> cpu_times = get_cpu_times();
        if (cpu_times.size() < 4)
            return false;
        idle_time = cpu_times[3];
        total_time = std::accumulate(cpu_times.begin(), cpu_times.end(), 0);
        return true;
    }


public:
    
    size_t previous_idle_time=0, previous_total_time=0;
    float GetCpuPercentage()
    {
        size_t idle_time, total_time;
        get_cpu_times(idle_time, total_time);
        const float idle_time_delta = idle_time - previous_idle_time;
        const float total_time_delta = total_time - previous_total_time;
        const float utilization = 100.0 * (1.0 - idle_time_delta / total_time_delta);
        // std::cout << utilization << '%' << std::endl;
        previous_idle_time = idle_time;
        previous_total_time = total_time;

        return utilization;
    }
    
    int get_used_mem()
    {
        std::string delimiter = "\n";

        //The full string looks like:
        //               total        used        free      shared  buff/cache   available
        // Mem:          31981        2301       27156         117        2523       29104
        // Swap:          2047           0        2047

        auto str = exec("free -m");

        //Skip the first line
        str = str.substr(str.find(delimiter)+1, str.length());
        
        //delete the last line
        str = str.substr(0, str.find(delimiter));

        //The line looks like:
        //Mem:          31981        2301       27156         117        2523       29104

        //Skip the "mem:"
        auto nextIndex = str.find(" ");
        str = str.substr(nextIndex, str.length());

        //navigate to the total (nummeric value) and skip the spaces
        while(str.find(" ") == 0)
        {
            str = str.substr(1, str.length());
        }

        //Skip the total value
        nextIndex = str.find(" ");
        str = str.substr(nextIndex, str.length());

        //navigate to the used (nummeric value) and skip the spaces
        while(str.find(" ") == 0)
        {
            str = str.substr(1, str.length());
        }

        //take the nummeric value (the wanted value)
        str = str.substr(0, str.find(" "));

        return stoi(str);
    } 


    void StartMeasurment(void (*FramesCallback)(Sample sample), int freq_ms)
    {

        _t = std::make_shared<std::thread>([this, FramesCallback, freq_ms]()
                                           {
                                                vector<int> v;
                                                Sample s {0};

                                                size_t previous_idle_time=0, previous_total_time=0;
                                                int sampleId = 0;
                                                for (size_t idle_time, total_time; get_cpu_times(idle_time, total_time); usleep(freq_ms*1000)) {
                                                    const float idle_time_delta = idle_time - previous_idle_time;
                                                    const float total_time_delta = total_time - previous_total_time;
                                                    const float utilization = 100.0 * (1.0 - idle_time_delta / total_time_delta);
                                                    // std::cout << utilization << '%' << std::endl;
                                                    previous_idle_time = idle_time;
                                                    previous_total_time = total_time;

                                                    //measure memory
                                                    int mem = get_used_mem();

                                                    s.Id = sampleId++;
                                                    s.Cpu_per = utilization;
                                                    s.Mem_MB = mem;

                                                    (*FramesCallback)(s);

                                                    if(stopRequested)
                                                        break;
                                                }
                                               
                                            });

    }
    void StopMeasurment()
    {
        stopRequested = true;
        _t->join();
    }

};



void sampleArrived(Sample s)
{
    cout << "========================" << endl;
    std::cout << "ID:\t" << s.Id << std::endl;
    std::cout << "CPU:\t" << s.Cpu_per << '%' << std::endl;
    std::cout << "Mem:\t" << s.Mem_MB << "MB" << std::endl;
    cout << "========================" << endl;

}

// int main(int argC, char** argV) 
// {  

//     SystemMonitor sm;

//     cout << "cpu= " << sm.GetCpuPercentage()<< " ";
//     sleep(1);

//     cout << "cpu= " << sm.GetCpuPercentage();
//     sleep(1);

//     cout << "cpu= " << sm.GetCpuPercentage();
//     sleep(1);


//     int frequency_ms = 200;
//     // sm.StartMeasurment(sampleArrived, frequency_ms);

//     // std::this_thread::sleep_for(std::chrono::seconds(2));

//     // sm.StopMeasurment();

//     return 0;

// }