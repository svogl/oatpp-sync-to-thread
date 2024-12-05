#pragma once

#include <regex>
#include <string>
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

#include OATPP_CODEGEN_BEGIN(ApiController)  /// <-- Begin Code-Gen

class CameraControllerCV : public oatpp::web::server::api::ApiController
{
   private:
    typedef CameraControllerCV __ControllerType;

   private:
    SyncApi* api;

    std::shared_ptr<oatpp::data::mapping::ObjectMapper> om;

   public:
    CameraControllerCV(OATPP_COMPONENT(
        std::shared_ptr<oatpp::web::mime::ContentMappers>, apiContentMappers))
        : oatpp::web::server::api::ApiController(apiContentMappers)
    {
        om = apiContentMappers->getDefaultMapper();
        OATPP_LOGd("CAMH", "CameraControllerCV: mapper serializing to {}/{}",
                   om->getInfo().mimeType, om->getInfo().mimeSubtype);
    }

    void setApi(SyncApi* api) { this->api = api; }

   public:  // endpoints:
    ENDPOINT_INFO(GetFrameCounterCV)
    {
        info->summary = "get framecounter2";
        info->description =
            "Get the next frame counter, sync via condition variable";
        info->addResponse<Int32>(Status::CODE_200, "application/json");
    }

    ENDPOINT_ASYNC("GET", "/sync", GetFrameCounterCV)
    {
        ENDPOINT_ASYNC_INIT(GetFrameCounterCV);

        virtual ~GetFrameCounterCV() { OATPP_LOGd("GFCV", "~bye"); }

        Action hasFrame()
        {
            SyncApi* api = controller->api;
            // OATPP_ASSERT(m_lockGuard.owns_lock())
            OATPP_LOGd("GFCV", "hasFrame {}", api->fc);
            auto dto = FrameDto::createShared();
            dto->fc = api->fc;
            dto->exposure = api->exposure;

            // // DO NOT do this here -> causes 'pure virtual method called'
            // oatpp::String s = controller->om->writeToString(dto);
            // // // std::string cs = *s;
            // // OATPP_LOGd("GFCV", "finish x  {} |{}|", __LINE__, s);

            return _return(
                controller->createDtoResponse(Status::CODE_200, dto));
        }

        SyncApi* api = nullptr;
        int old = -1;

        Action sync() 
        {
            api = controller->api;
            old = api->fc;

            // lock does not work in here, as sync() will be called multiple times
            oatpp::async::LockGuard guard(&api->lock);
            OATPP_LOGd("GFCV", "wait... {}", api->fc);
            return api->cv
                .waitFor(
                    guard,
                    [this]() noexcept {
                        OATPP_LOGd("GFCV", "wait? {} {}", api->fc, old);
                        return api->fc != old;
                    },
                    std::chrono::seconds(5))
                .next(yieldTo(&GetFrameCounterCV::hasFrame));
        }

        Action act() override
        {
            return yieldTo(&GetFrameCounterCV::sync);
        }
    };  // ENDPOINT_ASYNC("GET", "fc", GetFrameCounter
};

#include OATPP_CODEGEN_END(ApiController)  /// <-- End Code-Gen
