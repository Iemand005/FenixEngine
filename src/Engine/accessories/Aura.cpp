
#include "Aura.hpp"

#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <vector>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

#pragma pack(push, 1)
struct AuraInitReport
{
    BYTE reportId = 0x5A;
    BYTE cmd      = 0xBC;
    BYTE mode     = 0x01;
    BYTE reserved[61] = { 0 };
};

struct AuraColorReport
{
    BYTE reportId     = 0x5A;
    BYTE cmd          = 0xBC;
    BYTE mode         = 0x01;
    BYTE apply        = 0x01;
    BYTE reserved1[5] = { 0 };   // bytes 4-8
    BYTE r            = 0;
    BYTE g            = 0;
    BYTE b            = 0;
    BYTE reserved2[52] = { 0 };  // pad out to 64 total
};
#pragma pack(pop)

static_assert(sizeof(AuraInitReport)  == 64, "AuraInitReport must be exactly 64 bytes");
static_assert(sizeof(AuraColorReport) == 64, "AuraColorReport must be exactly 64 bytes");