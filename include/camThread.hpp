#pragma once

#include "syncApi.hpp"
/// this defines the interface to the data producer - a thread that routinely sends out data.


/** starts the camera thread
 * @param interFrameDelayMs - the delay in milli seconds to wait on each iteration
 */
extern void startCamThread(SyncApi* api, int interFrameDelayMs);

/** stop the thread
 * 
 */
extern void stopCamThread();

