#pragma once

#include "con_base.h"

namespace fglcd {

struct NoHwInit
{
    static FORCEINLINE void init() {}
};

template<typename TCmdCon, typename TDataCon, typename TDCxPin, typename TCSPin, typename TResetPin, typename THwInit = NoHwInit>
struct Interface
{
    typedef TDCxPin DCxPin;
    typedef TCSPin CSPin;
    typedef TResetPin ResetPin;
    typedef TCmdCon CmdCon;
    typedef TDataCon DataCon;
    typedef THwInit HwInit;
    
    static FORCEINLINE void init()
    {
        HwInit::init();
    }

    static_assert(has_con_tag<CmdCon>::value, "CmdCon is not a con");
    static_assert(has_con_tag<DataCon>::value, "DataCon is not a con");
    static_assert(has_pin_tag<DCxPin>::value, "DCxPin is not a pin");
    static_assert(has_pin_tag<CSPin>::value, "CSPin is not a pin");
    static_assert(has_pin_tag<ResetPin>::value, "ResetPin is not a pin");
};


} // end namespace fglcd
