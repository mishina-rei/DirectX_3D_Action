// EntityID.h

#pragma once

#include <cstdint>

namespace ECS 
{
    using EntityID = uint32_t;
    
    static constexpr EntityID INVALID_ENTITY_ID = -1;
}
