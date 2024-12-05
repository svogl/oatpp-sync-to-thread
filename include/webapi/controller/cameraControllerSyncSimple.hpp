#pragma once

#include <regex>
#include <string>
#include <thread>
#include <iostream>

#include "oatpp/web/protocol/http/outgoing/StreamingBody.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/json/Serializer.hpp"
#include "oatpp/utils/Conversion.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"

#include "globals.hpp"
#include "syncApi.hpp"

#include "webapi/dto/frameDto.hpp"

/** this example uses a separate coroutine to sync to the camera thread.
 * does not release the lock (why?) -- this effectively blocks the camera thread.
 */

class SimpleCoRo
    : public oatpp::async::CoroutineWithResult<
          SimpleCoRo,
          const std::shared_ptr<
              oatpp::web::server::api::ApiController::OutgoingResponse>&>
{
   private:
    SyncApi* api;
    oatpp::web::server::api::ApiController* ctrl;
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> om;
    int old;
    oatpp::async::LockGuard guard;
    oatpp::json::Serializer ser;

   public:
    SimpleCoRo(SyncApi* api, oatpp::web::server::api::ApiController* ctrl,
               std::shared_ptr<oatpp::data::mapping::ObjectMapper> om)
        : api(api), ctrl(ctrl), om(om), old(api->fc), guard(&api->lock)
    {
        OATPP_LOGd("CORO", "ctor... {}", api->fc);
        std::cout << "CTOR this=" << this << " " << std::this_thread::get_id() << std::endl;
    }

    virtual ~SimpleCoRo()
    {
        OATPP_LOGd("CORO", "~bye {}", api->fc);
        if (guard.owns_lock()) {
            guard.unlock();
        }
    }

    Action act() override
    {
        OATPP_LOGd("CORO", "wait... {} old {}", api->fc, old);
        return api->cv
            .waitFor(
                guard,
                [this]() noexcept {
                    OATPP_LOGd("CORO", "wait? {} != {} ?", api->fc, old);
                    return api->fc != old;
                },
                std::chrono::seconds(5))
            .next(yieldTo(&SimpleCoRo::hasFrame));
    }

    Action hasFrame()
    {
        // OATPP_ASSERT(guard.owns_lock())
        OATPP_LOGd("CORO", "hasFrame {}", api->fc);

        // DO NOT do this here -> enable this line to cause 'pure virtual method called'
        // oatpp::String s = om->writeToString(dto);

        char buf[64];
        snprintf(buf, sizeof(buf), "{\"fc\":%d,\"exposure\":%d}", api->fc,
                 api->exposure);
        return _return(ctrl->createResponse(
            oatpp::web::protocol::http::Status::CODE_200, buf));
    }
};

#include OATPP_CODEGEN_BEGIN(ApiController)  /// <-- Begin Code-Gen

class CameraControllerSyncSimple : public oatpp::web::server::api::ApiController
{
   private:
    typedef CameraControllerSyncSimple __ControllerType;

   private:
    SyncApi* api;

    std::shared_ptr<oatpp::data::mapping::ObjectMapper> om;

   public:
    CameraControllerSyncSimple(OATPP_COMPONENT(
        std::shared_ptr<oatpp::web::mime::ContentMappers>, apiContentMappers))
        : oatpp::web::server::api::ApiController(apiContentMappers)
    {
        om = apiContentMappers->getDefaultMapper();
        OATPP_LOGd("CAMH",
                   "CameraControllerSyncSimple: mapper serializing to {}/{}",
                   om->getInfo().mimeType, om->getInfo().mimeSubtype);
    }

    void setApi(SyncApi* api) { this->api = api; }

   public:  // endpoints:
    ENDPOINT_INFO(GetFrameCounterCV)
    {
        info->summary = "get framecounter synced";
        info->description =
            "Get the next frame counter, sync via condition variable";
        info->addResponse<Int32>(Status::CODE_200, "application/json");
    }

    ENDPOINT_ASYNC("GET", "/simpleSync", GetFrameCounterCV)
    {
        ENDPOINT_ASYNC_INIT(GetFrameCounterCV);

        virtual ~GetFrameCounterCV() { OATPP_LOGd("GFCV", "~bye"); }

        Action act() override
        {
            std::shared_ptr<SimpleCoRo> t = std::make_shared<SimpleCoRo>(
                controller->api, controller, controller->om);
            return t->act();
        }
    };  // ENDPOINT_ASYNC("GET", "fc", GetFrameCounter
};

#include OATPP_CODEGEN_END(ApiController)  /// <-- End Code-Gen
