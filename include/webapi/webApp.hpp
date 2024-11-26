
class SyncApi;

// start the thread running the oatpp server
extern void webAppStart(SyncApi* api = nullptr);

// stop the web thread
extern void webAppStop();

