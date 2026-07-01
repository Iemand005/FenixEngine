#pragma once

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

inline HANDLE OpenAura(USHORT vid, USHORT pid, USHORT page, USHORT usage)
{
    GUID guid;
    HidD_GetHidGuid(&guid);
    HDEVINFO devInfo = SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    SP_DEVICE_INTERFACE_DATA ifData{ sizeof(ifData) };
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr, &guid, i, &ifData); i++)
    {
        DWORD size = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, nullptr, 0, &size, nullptr);
        std::vector<BYTE> buf(size);
        auto detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buf.data();
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, detail, size, nullptr, nullptr)) continue;

        HANDLE h = CreateFileW(detail->DevicePath, GENERIC_WRITE | GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES attr{ sizeof(attr) };
        HidD_GetAttributes(h, &attr);

        PHIDP_PREPARSED_DATA pp;
        HIDP_CAPS caps{};
        if (attr.VendorID == vid && attr.ProductID == pid && HidD_GetPreparsedData(h, &pp))
        {
            HidP_GetCaps(pp, &caps);
            HidD_FreePreparsedData(pp);
            if (caps.UsagePage == page && caps.Usage == usage) return h;
        }
        CloseHandle(h);
    }
    return nullptr;
}

inline void SetColor(HANDLE dev, BYTE r, BYTE g, BYTE b)
{
    AuraInitReport init;
    HidD_SetFeature(dev, &init, sizeof(init));

    AuraColorReport report;
    report.r = r;
    report.g = g;
    report.b = b;
    HidD_SetFeature(dev, &report, sizeof(report));
}