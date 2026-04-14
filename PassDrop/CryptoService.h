#pragma once
#include <vector>
#include <string>
#include <cstdint>

class CryptoService {
public:
    static CryptoService& getInstance();
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext);
    std::wstring generatePassword(int length = 16);
private:
    CryptoService();
    std::vector<uint8_t> getMachineKey();
    std::vector<uint8_t> m_machineKey;
};