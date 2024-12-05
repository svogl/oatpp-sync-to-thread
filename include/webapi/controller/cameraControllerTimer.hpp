#pragma once

#include <regex>
#include <iostream>

#include "oatpp/web/protocol/http/outgoing/StreamingBody.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/utils/Conversion.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"

#include "globals.hpp"
#include "syncApi.hpp"

#include "webapi/dto/frameDto.hpp"

/** sync camera thread and web frontend via async timeouts.
 * Data from the frameDto is serialized in a readcallback.
 * Ugly, but works.
 */

// based on https://github.com/lganzzzo/canchat/blob/master/server/src/controller/FileController.hpp

#include OATPP_CODEGEN_BEGIN(ApiController)  /// <-- Begin Code-Gen

class TimerReadCallback;

class CameraControllerTimer : public oatpp::web::server::api::ApiController
{
    friend class
        TimerReadCallback;  // for now convenience for the object mapper
   private:
    typedef CameraControllerTimer __ControllerType;

    SyncApi* api;

    std::shared_ptr<oatpp::data::mapping::ObjectMapper> om;

   private:
    // OATPP_COMPONENT(std::shared_ptr<Lobby>, lobby);

   public:
    CameraControllerTimer(OATPP_COMPONENT(
        std::shared_ptr<oatpp::web::mime::ContentMappers>, apiContentMappers))
        : oatpp::web::server::api::ApiController(apiContentMappers)
    {
        om = apiContentMappers->getDefaultMapper();
        OATPP_LOGd("CAMH", "CameraController: mapper serializing to {}/{}",
                   om->getInfo().mimeType, om->getInfo().mimeSubtype);
    }

    void setApi(SyncApi* api) { this->api = api; }

   public:
    ENDPOINT_INFO(GetFrameCounter)
    {
        info->summary = "get framecounter";
        info->description = "Get the next frame counter, sync via timer";
        info->addResponse<Int32>(Status::CODE_200, "application/json");
    }
    ENDPOINT_ASYNC("GET", "/timer", GetFrameCounter)
    {
        ENDPOINT_ASYNC_INIT(GetFrameCounter);

        Action act() override
        {
            if (!controller->api) {
                return _return(controller->createResponse(Status::CODE_404));
            }

            auto body = std::make_shared<
                oatpp::web::protocol::http::outgoing::StreamingBody>(
                std::make_shared<TimerReadCallback>(*controller->api,
                                                    controller));

            auto response =
                OutgoingResponse::createShared(Status::CODE_200, body);
            response->putHeader("content-type", "application/json");

            return _return(response);
        }
    };  // ENDPOINT_ASYNC("GET", "fc", GetFrameCounter

    ENDPOINT_ASYNC("GET", "version", GetVersion)
    {
        ENDPOINT_ASYNC_INIT(GetVersion);

        Action act() override
        {
            return _return(
                controller->createResponse(Status::CODE_200, version()));
        }
    };

    ENDPOINT_ASYNC("GET", "stop", Stop)
    {
        ENDPOINT_ASYNC_INIT(Stop);

        Action act() override
        {
            controller->api->keepRunning = false;
            return _return(
                controller->createResponse(Status::CODE_200, "leaving"));
        }
    };
};

/** timer read callback class - spins until new data is available 
 * from the camera thread, then fills the buffer.
 * 
 */
class TimerReadCallback : public oatpp::data::stream::ReadCallback
{
   private:
    SyncApi& api;
    int old = -1;
    const CameraControllerTimer* self;
    int len = 0;

   public:
    TimerReadCallback(SyncApi& api, const CameraControllerTimer* self)
        : api(api), old(api.fc), self(self)
    {
    }

    oatpp::v_io_size read(void* buffer, v_buff_size count,
                          oatpp::async::Action& action) override
    {
        // timer strategy adapted from oatpp/test/oatpp/web/app/ControllerAsync.hpp
        // check if the frame counter has changed
        if (api.fc == old) {
            // limits to 50fps; better would be to compute expected arrival time by storing framerate&lastTs
            action = oatpp::async::Action::createWaitRepeatAction(
                20 * 1000 + oatpp::Environment::getMicroTickCount());
            OATPP_LOGd("CAMH", "tick");
            return 0;
        }
        OATPP_LOGd("CAMH", "ticked {} dlen={}", api.fc, len);
        if (len) {
            // second iteration with new contents - tell the body-builder we are done:
            action = oatpp::async::Action::createActionByType(
                oatpp::async::Action::TYPE_NONE);
            return 0;
        }
        // first iteration with new contents - new data available, let's prepare an answer

        auto dto = FrameDto::createShared();
        dto->fc = api.fc;
        dto->exposure = api.exposure;

        oatpp::String s = self->om->writeToString(dto);

        const std::string& ss = *s;
        len = ss.size();

        memcpy(buffer, ss.c_str(), len + 1);
        return len;
    }
};

#include OATPP_CODEGEN_END(ApiController)  /// <-- End Code-Gen
