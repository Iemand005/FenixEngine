#pragma once

#include <memory>

class Aura {

    struct Impl;
    std::unique_ptr<Impl> impl;
    HANDLE dev = NULL;

    HANDLE OpenAura(USHORT vid, USHORT pid, USHORT page, USHORT usage)
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

public:

    Aura() {
        this->dev = OpenAura(0x0B05, 0x19B6, 0xFF31, 0x76);
    }

    ~Aura() {
        if (dev) CloseHandle(dev);
    }

    bool IsOpen() const { return dev != NULL; }

    bool SetColor(BYTE r, BYTE g, BYTE b)
    {
        if (!IsOpen()) return false;
        AuraInitReport init;
        HidD_SetFeature(dev, &init, sizeof(init));

        AuraColorReport report;
        report.r = r;
        report.g = g;
        report.b = b;
        HidD_SetFeature(dev, &report, sizeof(report));
        return true; // TODO: might not be successfull but wharever
    }

};