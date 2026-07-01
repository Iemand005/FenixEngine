#pragma once

#include <memory>

class Aura {

    struct Impl;
    std::unique_ptr<Impl> impl;



public:

    Aura();

    ~Aura();

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