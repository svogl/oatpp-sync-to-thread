#include <iostream>
#include <thread>
#include <stdlib.h>

#include "camThread.hpp"
#include "oatpp/base/Log.hpp"

using namespace std;

static int ms = 1000;
static bool keepRunning = true;
static thread camThread;
// static SyncApi* api = nullptr;

static void run(SyncApi* api)
{
    do {
        // so some stuff, tell everybody that there's work:
        {
            std::lock_guard<oatpp::async::Lock> guard(api->lock);
            api->fc++;

            int rnd = 0;
            // generate a gaussian distribution..
            for (int i = 0; i < 6; i++) {
                rnd += rand() % 0xff;
            }
            api->exposure = 512 + rnd / 6;
            OATPP_LOGi("CAM", "beep\t{}\t{}", api->fc, api->exposure);
        }
        api->cv.notifyAll();

        // back to sleep...
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    } while (keepRunning);
}

void startCamThread(SyncApi* api, int interFrameDelayMs)
{
    srand(0xdeadbeef);

    ms = interFrameDelayMs;
    camThread = std::thread(run, api);
}

void stopCamThread()
{
    keepRunning = false;
    camThread.join();  // pauses until first finishes
}
