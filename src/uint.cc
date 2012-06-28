#include "uint.h"

#include <inttypes.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

UInt :: UInt ()
{
    this->bits  = 0;
    this->value = 0;
}

UInt :: UInt (int bits)
{
    this->bits  = bits;
    this->value = 0;
}

UInt :: UInt (int bits, uint64_t value)
{
    this->bits  = bits;
    this->value = value;
}

__uint128_t UInt :: g_value128 () const
{
    if (bits == 128)
        return value;

    __uint128_t mask = 1;
    mask <<= bits;
    mask -= 1;
    return value & mask;
}

__int128_t UInt :: g_svalue128 () const
{
    switch (bits) {
    case 1   : return value & 1;
    case 8   : return (int8_t)  value;
    case 16  : return (int16_t) value;
    case 32  : return (int32_t) value;
    case 64  : return (int64_t) value;
    case 128 : return (__int128_t) value;
    }
    throw std::runtime_error("invalid number of bits to UInt :: g_svalue()");
    return 0;
}

UInt UInt :: sign_extend (int bits) const
{
    UInt result(bits);
    result.value = g_svalue128();
    return result;
}

UInt UInt :: extend (int bits) const
{
    UInt result(bits);
    result.value = g_value128();
    return result;
}

std::string UInt :: str () const
{
    uint64_t hi = (g_value128() >> 64);
    uint64_t lo = g_value128();
    std::stringstream ss;
    if (bits == 128)
        ss << std::hex << "0x"
           << std::setfill('0') << std::setw(16) << hi
           << std::setfill('0') << std::setw(16) << lo;
    else if (bits == 1) {
        ss << std::hex << std::setw(1) << "0x" << std::setw(1) << lo;
    }
    else
        ss << std::hex << "0x"
           << std::setfill('0') << std::setw(bits/4) << lo;
    return ss.str();
}

#define UINTOPERATOR(OPER) \
UInt UInt :: operator OPER (const UInt & rhs) const \
{ \
    UInt result(this->bits); \
    result.value = this->g_value128() OPER rhs.g_value128(); \
    return result; \
}

UINTOPERATOR(+)
UINTOPERATOR(-)
UINTOPERATOR(*)
UINTOPERATOR(/)
UINTOPERATOR(%)
UINTOPERATOR(&)
UINTOPERATOR(^)
UINTOPERATOR(|)
UINTOPERATOR(<<)
UINTOPERATOR(>>)

UInt UInt :: operator ~ () const
{
    return UInt(this->bits, ~(this->value));
}

#define UINTCMPOPERATOR(OPER) \
bool UInt :: operator OPER (const UInt & rhs) const \
{ \
    return this->g_value128() OPER rhs.g_value128(); \
}

UINTCMPOPERATOR(<)
UINTCMPOPERATOR(>)
UINTCMPOPERATOR(<=)
UINTCMPOPERATOR(>=)
UINTCMPOPERATOR(==)
UINTCMPOPERATOR(!=)

bool UInt :: cmpLts (const UInt & rhs) const
{
    return (g_svalue128() < rhs.g_svalue128());
}

bool UInt :: cmpLes (const UInt & rhs) const
{
    return (g_svalue128() <= rhs.g_svalue128());
}