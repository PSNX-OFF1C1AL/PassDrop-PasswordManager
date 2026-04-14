#include "DataStore.h"
#include "CryptoService.h"
#include <fstream>
#include <algorithm>
#include <chrono>

DataStore::DataStore() {}

bool DataStore::loadFromFile(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), {});
    file.close();
    auto dec = CryptoService::getInstance().decrypt(data);
    return deserialize(dec);
}

bool DataStore::saveToFile(const std::wstring& path) {
    auto data = serialize();
    auto enc = CryptoService::getInstance().encrypt(data);
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write((char*)enc.data(), enc.size());
    m_modified = false;
    return true;
}

std::vector<Account> DataStore::searchAccounts(const std::wstring& query) const {
    std::vector<Account> res;
    std::wstring q = query; std::transform(q.begin(), q.end(), q.begin(), ::towlower);
    for (auto& a : m_accounts) {
        std::wstring t = a.title; std::transform(t.begin(), t.end(), t.begin(), ::towlower);
        if (t.find(q) != std::wstring::npos) res.push_back(a);
    }
    return res;
}

Account DataStore::getAccount(int id) const {
    auto it = std::find_if(m_accounts.begin(), m_accounts.end(), [id](auto& a) { return a.id == id; });
    return it != m_accounts.end() ? *it : Account{};
}

bool DataStore::addAccount(Account& acc) {
    acc.id = m_nextId++;
    acc.createdAt = acc.updatedAt = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    m_accounts.push_back(acc);
    m_modified = true;
    return true;
}

bool DataStore::updateAccount(const Account& acc) {
    auto it = std::find_if(m_accounts.begin(), m_accounts.end(), [&](auto& a) { return a.id == acc.id; });
    if (it == m_accounts.end()) return false;
    *it = acc;
    it->updatedAt = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    m_modified = true;
    return true;
}

bool DataStore::deleteAccount(int id) {
    auto it = std::find_if(m_accounts.begin(), m_accounts.end(), [id](auto& a) { return a.id == id; });
    if (it == m_accounts.end()) return false;
    m_accounts.erase(it);
    m_modified = true;
    return true;
}

// Serialization helpers
static void writeU32(std::vector<uint8_t>& d, uint32_t v) { for (int i = 0;i < 4;++i) d.push_back((v >> (i * 8)) & 0xFF); }
static void writeU64(std::vector<uint8_t>& d, uint64_t v) { for (int i = 0;i < 8;++i) d.push_back((v >> (i * 8)) & 0xFF); }
static void writeStr(std::vector<uint8_t>& d, const std::wstring& s) {
    writeU32(d, (uint32_t)(s.size() * sizeof(wchar_t)));
    d.insert(d.end(), (uint8_t*)s.data(), (uint8_t*)s.data() + s.size() * sizeof(wchar_t));
}
static void writeBlob(std::vector<uint8_t>& d, const std::vector<uint8_t>& b) {
    writeU32(d, (uint32_t)b.size());
    d.insert(d.end(), b.begin(), b.end());
}
static uint32_t readU32(const uint8_t*& p, const uint8_t* e) { if (p + 4 > e) return 0; uint32_t v = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); p += 4; return v; }
static uint64_t readU64(const uint8_t*& p, const uint8_t* e) { if (p + 8 > e) return 0; uint64_t v = 0; for (int i = 0;i < 8;++i) v |= ((uint64_t)p[i] << (i * 8)); p += 8; return v; }
static std::wstring readStr(const uint8_t*& p, const uint8_t* e) {
    uint32_t sz = readU32(p, e); if (p + sz > e) return L"";
    std::wstring s((wchar_t*)p, sz / sizeof(wchar_t)); p += sz; return s;
}
static std::vector<uint8_t> readBlob(const uint8_t*& p, const uint8_t* e) {
    uint32_t sz = readU32(p, e); if (p + sz > e) return {};
    std::vector<uint8_t> b(p, p + sz); p += sz; return b;
}

std::vector<uint8_t> DataStore::serialize() const {
    std::vector<uint8_t> d;
    writeU32(d, 0x50445250); writeU32(d, 1); writeU32(d, (uint32_t)m_accounts.size());
    for (auto& a : m_accounts) {
        writeU32(d, a.id); writeStr(d, a.title); writeStr(d, a.url); writeStr(d, a.icon);
        writeBlob(d, a.loginEnc); writeBlob(d, a.passwordEnc);
        writeStr(d, a.extra1Label); writeBlob(d, a.extra1DataEnc);
        writeStr(d, a.extra2Label); writeBlob(d, a.extra2DataEnc);
        writeU64(d, a.createdAt); writeU64(d, a.updatedAt);
    }
    return d;
}

bool DataStore::deserialize(const std::vector<uint8_t>& d) {
    const uint8_t* p = d.data(); const uint8_t* e = p + d.size();
    if (readU32(p, e) != 0x50445250 || readU32(p, e) != 1) return false;
    uint32_t cnt = readU32(p, e); m_accounts.clear(); m_accounts.reserve(cnt);
    int maxId = 0;
    for (uint32_t i = 0; i < cnt; ++i) {
        Account a;
        a.id = readU32(p, e); a.title = readStr(p, e); a.url = readStr(p, e); a.icon = readStr(p, e);
        a.loginEnc = readBlob(p, e); a.passwordEnc = readBlob(p, e);
        a.extra1Label = readStr(p, e); a.extra1DataEnc = readBlob(p, e);
        a.extra2Label = readStr(p, e); a.extra2DataEnc = readBlob(p, e);
        a.createdAt = readU64(p, e); a.updatedAt = readU64(p, e);
        m_accounts.push_back(a); if (a.id > maxId) maxId = a.id;
    }
    m_nextId = maxId + 1;
    return true;
}