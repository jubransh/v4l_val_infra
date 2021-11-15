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
#include <unordered_map>

using namespace std;

class A
{
private:
public:
};
void stuckForEver()
{
    while (true)
    {
        usleep(500000);
        cout << "I'm stuck" << endl;
    }
}

int main(int argC, char **argV)
{
    typedef unordered_map<string, pthread_t> ThreadMap;
    ThreadMap tm;
    string t_name = "stuck_forever";

    thread t(stuckForEver);

    tm[t_name] = t.native_handle();
    t.detach();

    usleep(5000000);
    cout << "Stopping thread" << endl;

    //Stopping the thread
    ThreadMap::const_iterator it = tm.find(t_name);
    if (it != tm.end())
    {
        pthread_cancel(it->second);
        tm.erase(t_name);
    }
    usleep(5000000);

    return 0;
}