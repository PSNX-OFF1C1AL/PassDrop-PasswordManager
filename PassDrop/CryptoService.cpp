#include "CryptoService.h"
#include <random>

// Статический ключ (переносимый между ПК)
static constexpr uint8_t STATIC_KEY[32] = {
    0x7E, 0x92, 0x1A, 0x4F, 0xB3, 0xC8, 0x6D, 0x12,
    0xE5, 0x9A, 0x2B, 0x7F, 0x41, 0xD8, 0x36, 0x95,
    0x8C, 0x6E, 0x3A, 0x1D, 0x54, 0xB7, 0xE2, 0x9F,
    0x13, 0x67, 0xCA, 0x4B, 0x80, 0xD5, 0x29, 0xF1
};

CryptoService::CryptoService() {}
CryptoService& CryptoService::getInstance() { static CryptoService inst; return inst; }

std::vector<uint8_t> CryptoService::encrypt(const std::vector<uint8_t>& plaintext) {
    if (plaintext.empty()) return {};
    std::vector<uint8_t> out; out.reserve(plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i)
        out.push_back(plaintext[i] ^ STATIC_KEY[i % 32]);
    return out;
}

std::vector<uint8_t> CryptoService::decrypt(const std::vector<uint8_t>& ciphertext) {
    return encrypt(ciphertext);
}

std::wstring CryptoService::generatePassword(int length) {
    std::wstring chars = L"ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789!@#$%^&*_-+=";
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)chars.size() - 1);
    std::wstring pwd;
    for (int i = 0; i < length; ++i) pwd += chars[dis(gen)];
    return pwd;
}