#pragma once

#include <memory>

class Aura {

    struct Impl;
    std::unique_ptr<Impl> impl;



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