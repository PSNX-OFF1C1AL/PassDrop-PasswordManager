#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Account {
    int id = -1;
    std::wstring title;
    std::wstring url;
    std::wstring icon;
    std::vector<uint8_t> loginEnc;
    std::vector<uint8_t> passwordEnc;
    std::wstring extra1Label;
    std::vector<uint8_t> extra1DataEnc;
    std::wstring extra2Label;
    std::vector<uint8_t> extra2DataEnc;
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
    int colorIndex = 0;

    bool isValid() const { return id >= 0 && !title.empty(); }
};