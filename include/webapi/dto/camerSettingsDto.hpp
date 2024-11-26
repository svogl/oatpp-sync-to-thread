#pragma once

#include "oatpp/Types.hpp"
#include "oatpp/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

/// camera settings - minimal example
class CameraSettings : public oatpp::DTO
{
    DTO_INIT(CameraSettings, DTO)

    /// frame counter, monotonic increasing
    DTO_FIELD(Int32, fc);

    /// exposure time - some randomish value
    DTO_FIELD(Int32, exposure);
};

#include OATPP_CODEGEN_END(DTO)
