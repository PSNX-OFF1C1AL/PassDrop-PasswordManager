#pragma once
#include <vector>
#include <string>
#include "Account.h"

class DataStore {
public:
    DataStore();
    bool loadFromFile(const std::wstring& path);
    bool saveToFile(const std::wstring& path);
    std::vector<Account> getAllAccounts() const { return m_accounts; }
    std::vector<Account> searchAccounts(const std::wstring& query) const;
    Account getAccount(int id) const;
    bool addAccount(Account& acc);
    bool updateAccount(const Account& acc);
    bool deleteAccount(int id);
    bool isModified() const { return m_modified; }
private:
    std::vector<Account> m_accounts;
    int m_nextId = 1;
    bool m_modified = false;
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
};