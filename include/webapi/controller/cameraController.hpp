#pragma once

#include <regex>
#include <iostream>

#include "oatpp/web/protocol/http/outgoing/StreamingBody.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/utils/Conversion.hpp"

#include "oatpp/macro/codegen.hpp"
#include "oatpp/macro/component.hpp"

#include "syncApi.hpp"

// based on https://github.com/lganzzzo/canchat/blob/master/server/src/controller/FileController.hpp

#include OATPP_CODEGEN_BEGIN(ApiController)  /// <-- Begin Code-Gen

class CameraController : public oatpp::web::server::api::ApiController
{
   private:
    typedef CameraController __ControllerType;

    static SyncApi* api;

   private:
    // OATPP_COMPONENT(std::shared_ptr<Lobby>, lobby);

   public:
    CameraController(OATPP_COMPONENT(
        std::shared_ptr<oatpp::web::mime::ContentMappers>, apiContentMappers))
        : oatpp::web::server::api::ApiController(apiContentMappers)
    {
    }

    void setApi(SyncApi* api) { this->api = api; }

   public:
    ENDPOINT_INFO(GetFrameCounter)
    {
        info->summary = "get framecounter";
        info->description = "Get the next frame counter";
        info->addResponse<Int32>(Status::CODE_200, "application/json");
    }
    ENDPOINT_ASYNC("GET", "/fc", GetFrameCounter)
    {
        ENDPOINT_ASYNC_INIT(GetFrameCounter)

        class SyncApiCallback : public oatpp::data::stream::ReadCallback
        {
           private:
            // std::shared_ptr<File::Subscriber> m_subscriber;

           private:
            // OATPP_COMPONENT(std::shared_ptr<Statistics>, m_statistics);
            SyncApi& api;
            int old = -1;
            const CameraController* self;

           public:
            SyncApiCallback(SyncApi& api, const CameraController* self)
                : api(api), old(api.fc), self(self)
            {
            }

            oatpp::v_io_size read(void* buffer, v_buff_size count,
                                  oatpp::async::Action& action) override
            {
                // strategy adapted from oatpp/test/oatpp/web/app/ControllerAsync.hpp
                if (api.fc == old) {
                    // limits to 50fps; better would be to compute expected arrival time
                    action = oatpp::async::Action::createWaitRepeatAction(
                        20 * 1000 + oatpp::Environment::getMicroTickCount());
                    std::cout << "tick" << std::endl;
                    return 0;
                }

                oatpp::Int32 fc = api.fc;

                // String s = controller->writeToString(fc);
                String s = "123";
                const std::string& ss = *s;

                // assert count > ss.size();
                // get data & send
                memcpy(buffer, ss.c_str(), ss.size());
                return ss.size();
            }
        };

        Action act() override
        {
            if (!api) {
                return _return(controller->createResponse(Status::CODE_404));
            }

            auto body = std::make_shared<
                oatpp::web::protocol::http::outgoing::StreamingBody>(
                std::make_shared<SyncApiCallback>(*api, controller));

            auto response =
                OutgoingResponse::createShared(Status::CODE_200, body);
            response->putHeader("content-type", "application/json");

            // auto response = controller->createResponse(Status::CODE_200);

            return _return(response);
        }
    };  // ENDPOINT_ASYNC("GET", "fc", GetFrameCounter

    ENDPOINT("GET", "stop", stop)
    {
        api->keepRunning = false;
        return createResponse(Status::CODE_200, "leaving");
    }
};

#include OATPP_CODEGEN_END(ApiController)  /// <-- End Code-Gen
