#pragma once

#include <stdint.h>


// Adapted from http://www.arklyffe.com/main/2010/08/29/xorshift-pseudorandom-number-generator/

class TinyRng8
{
public:
    TinyRng8() : _state((uint8_t)(uintptr_t)this) {}
    TinyRng8(uint8_t x) : _state(x) {}
    inline void init(uint8_t x) { _state = x; }
    inline void mix(uint8_t x) { _state += x; }
    uint8_t operator()()
    {
        uint8_t s = _state;
        s ^= (s << 1);
        s ^= (s >> 1);
        s ^= (s << 2);
        _state = s;
        return s - 1;
    }
private:
    uint8_t _state;
};

class TinyRng16
{
public:
    TinyRng16() : _state((uint16_t)(uintptr_t)this) {}
    TinyRng16(uint16_t x) : _state(x) {}
    inline void init(uint16_t x) { _state = x; }
    inline void mix(uint16_t x) { _state += x; }
    uint16_t operator()()
    {
        uint16_t s = _state;
        s ^= (s << 1);
        s ^= (s >> 1);
        s ^= (s << 14);
        _state = s;
        return s - 1;
    }
private:
    uint16_t _state;
};
