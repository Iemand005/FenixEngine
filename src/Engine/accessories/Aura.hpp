#pragma once

#include <memory>

class Aura {

    struct Impl;
    std::unique_ptr<Impl> impl;

public:

    Aura();

    ~Aura();

    bool IsOpen() const;

    bool SetColor(char r, char g, char b);
};