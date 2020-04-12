#pragma once

#include "mcu.h"
#include "util.h"
#include "interface.h"

namespace fglcd {

template<typename CmdConnection, typename CmdTrigger>
struct Command_Exclusive
{
    typedef bool is_command_tag;
    static_assert(has_trigger_tag<CmdTrigger>::value, "CmdTrigger is not trigger");
    static_assert(has_connection_tag<CmdConnection>::value, "CmdConnection is not connection");
    typedef typename CmdConnection::value_type value_type;

    static FORCEINLINE void begin(value_type cmd)
    {
        CmdTrigger::set();
        CmdConnection::send(cmd);
        CmdTrigger::clear();
    }
    static FORCEINLINE void param(value_type cmd)
    {
        CmdConnection::send(cmd);
    }
};

template<typename Cmd>
struct ChipCtrl
{
    static_assert(has_command_tag<Cmd>::value, "CMD type does not have is_command_tag");
    typedef typename Cmd::value_type CmdType;

    template<typename FROM>
    static void sendtable(typename FROM::VoidPointer pp)
    {
        typename FROM::Pointer p = static_cast<typename FROM::Pointer>(pp);
        for(u8 c; (c = _readinc<FROM>(p)) ; )
        {
            Cmd::begin(_readinc<FROM>(p));
            for(u8 i = 1; i < c; ++i)
                sendparam8(_readinc<FROM>(p));
        }
    }
    static FORCEINLINE void sendcmd(CmdType com)
    {
        Cmd::begin(com);
    }
    static FORCEINLINE void sendparam8(u8 x)
    {
        Cmd::param(x);
    }
    static FORCEINLINE void sendparam16(u16 a)
    {
        union
        {
            struct { u8 lo, hi; } pair;
            u16 both;
        } u;
        u.both = a;
        sendparam8(u.pair.hi);
        sendparam8(u.pair.lo);
    }


private:
    template<typename FROM>
    static FORCEINLINE CmdType _readinc(typename FROM::Pointer& p)
    {
        return FROM::template read<CmdType>(p++);
    }
};

template<typename TSpecs, typename Iface>
struct ChipBase : public TSpecs
{
    typedef TSpecs Specs;
    typedef SizeConfig<Specs::LONG_SIZE, Specs::SHORT_SIZE> _SizeCfg;
    typedef typename _SizeCfg::type TotalPixelType;
    static const TotalPixelType TOTAL_PIXELS = _SizeCfg::total;

    typedef typename Iface::ResetPin ResetPin;
    typedef typename Iface::DCxPin DCxPin;
    typedef typename Iface::CSPin CSPin;

    typedef typename Iface::CmdCon _CmdCon;
    typedef typename Iface::DataCon _DataCon;
    typedef typename _CmdCon::TriggerPin _CmdTriggerPin;
    typedef typename _DataCon::TriggerPin _DataTriggerPin;

    typedef typename Specs::template ResetTriggerType<ResetPin> ResetTrigger;
    typedef typename Specs::template DCxTriggerType<DCxPin> DCxTrigger;
    typedef typename Specs::template WrxCmdTriggerType<_CmdTriggerPin> _WrxCmdTrigger;
    typedef typename Specs::template WrxDataTriggerType<_DataTriggerPin> _WrxDataTrigger;
    typedef typename Specs::template CSTriggerType<CSPin> CSTrigger;

    typedef Connection<_CmdCon, _WrxCmdTrigger> CmdConnection;
    typedef Connection<_DataCon, _WrxDataTrigger> DataConnection;

    typedef Command_Exclusive<CmdConnection, DCxTrigger> Cmd; // TODO: should be param

    typedef ChipCtrl<Cmd> Ctrl;

    static void init()
    {
        ResetTrigger::clear();
        CSTrigger::clear();
        DCxTrigger::clear();
        _CmdCon::Port::makeOutput();
        _DataCon::Port::makeOutput();
        ResetPin::makeOutput();
        DCxPin::makeOutput();
        CSPin::makeOutput();
        _CmdTriggerPin::makeOutput();
        _DataTriggerPin::makeOutput();

        Iface::init();
    }

    static FORCEINLINE void reset() { ResetTrigger::trigger(); }
    static FORCEINLINE void enableCS() { CSTrigger::set(); }
    static FORCEINLINE void disableCS() { CSTrigger::clear(); }
};


} // end namespace fglcd
