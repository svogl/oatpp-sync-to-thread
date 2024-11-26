#include <iostream>
#include <string>
#include <thread>

#include "syncApi.hpp"

#include "camera/camThread.hpp"

#include "webapi/webApp.hpp"

using namespace std;

/**
 *  main; starts up the processing threads & idles mainly.
 */
int main(int argc, const char *argv[])
{
    int ms = 1000;

    for (int i = 1; i < argc; i++) { // poor man's argument parser...
        if (string(argv[i]) == "-m") {
            // try to parse delay
            ms = atoi(argv[++i]);
        } // else unknownArgs++
    }

    cout << "starting up, delay set to " << ms << endl;

    SyncApi api;

    webAppStart(&api);
    startCamThread(&api, ms);

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // rotate logs, compute statistics, wait for input on stdin,...
    } while (api.keepRunning);

    cout << "stopping" << endl;
    webAppStop();
    stopCamThread();
    cout << "done" << endl;

    return 0;
}