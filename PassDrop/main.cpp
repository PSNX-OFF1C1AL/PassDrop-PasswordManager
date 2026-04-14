/*
 * PassDrop - Modern Password Manager
 * Copyright (C) 2026 Maksutov Vladislav Vitalievich (Pichan)
 * License: GNU GPL v3
 */

#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include "resource.h"
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <codecvt>
#include <ctime>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "DataStore.h"
#include "CryptoService.h"
#include "Account.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

 // Filter callback for InputText - only allow characters supported by current font
static int FilterSupportedChars(ImGuiInputTextCallbackData* data) {
    ImWchar c = (ImWchar)data->EventChar;

    // Allow control characters (backspace, enter, etc.)
    if (c < 32) return 0;

    // Allowed Unicode ranges (customize as needed)
    bool allowed =
        (c >= 0x0020 && c <= 0x007F) || // Basic Latin
        (c >= 0x0400 && c <= 0x04FF) || // Cyrillic
        (c >= 0x3040 && c <= 0x309F) || // Hiragana
        (c >= 0x30A0 && c <= 0x30FF) || // Katakana
        (c >= 0x4E00 && c <= 0x9FFF);   // CJK Ideographs

    return allowed ? 0 : 1;
}

 // ==================== Constants ====================
#define ID_TRAY_SHOW    1001
#define ID_TRAY_EXIT    1002
#define WM_TRAYICON     (WM_USER + 1)

static constexpr int CLIPBOARD_TIMEOUT_SEC = 30;
bool g_showNewConfirm = false;

bool g_showExportWarning = false;
bool g_exportPending = false;

// Emoji icons
static const wchar_t* ICONS[] = {
    L"*", L"@", L"~", L"#", L"&", L"+", L"=", L"$", L"%", L"!",
    L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"0"
};
static const int ICON_COUNT = sizeof(ICONS) / sizeof(ICONS[0]);

// Colors for accounts
static const ImVec4 COLORS[] = {
    ImVec4(1.00f, 1.00f, 1.00f, 1.0f), // Default
    ImVec4(1.00f, 0.30f, 0.30f, 1.0f), // Red
    ImVec4(1.00f, 0.60f, 0.20f, 1.0f), // Orange
    ImVec4(1.00f, 0.90f, 0.10f, 1.0f), // Yellow
    ImVec4(0.30f, 0.80f, 0.30f, 1.0f), // Green
    ImVec4(0.30f, 0.60f, 1.00f, 1.0f), // Blue
    ImVec4(0.70f, 0.30f, 1.00f, 1.0f), // Purple
};
static const int COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);
static const char* COLOR_NAMES[] = {
    "Default", "Red", "Orange", "Yellow", "Green", "Blue", "Purple"
};

// ==================== Global Variables ====================
HWND g_hWnd = nullptr;
DataStore g_dataStore;
std::vector<Account> g_accounts;
char g_search[256] = "";
std::string g_currentFile;
bool g_running = true;
bool g_maximized = false;
WINDOWPLACEMENT g_prevPlacement = { sizeof(WINDOWPLACEMENT) };
bool g_darkTheme = true;
bool g_showExtra1 = true;
bool g_showExtra2 = true;
static std::unordered_map<int, bool> g_showPassMap;

// Password generator settings
int g_pwdLength = 16;
bool g_pwdUpper = true;
bool g_pwdLower = true;
bool g_pwdDigits = true;
bool g_pwdSpecial = true;

bool g_showImportWarning = false;
bool g_importPending = false;

void DoExportToCSV();
void DoImportFromCSV();

// Tray
NOTIFYICONDATAW g_nid = {};

// DirectX
ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
IDXGISwapChain* g_swapChain = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;

// Clipboard
std::string g_lastCopiedHash;
std::chrono::steady_clock::time_point g_lastCopyTime;

// Add/Edit buffers
struct AddEditBuffers {
    char title[256] = "";
    char url[256] = "";
    char login[256] = "";
    char pass[256] = "";
    char e1label[256] = "";
    char e1data[256] = "";
    char e2label[256] = "";
    char e2data[256] = "";
    int iconIndex = 0;
    int colorIndex = 0;
};
AddEditBuffers g_buf;

// Popup states
bool g_showAddEdit = false;
bool g_isEdit = false;
Account g_editAccount;
bool g_showDeleteConfirm = false;
int g_deleteId = -1;
bool g_showAbout = false;
bool g_showPasswordGenerator = false;

// Notification popup
bool g_showNotification = false;
std::string g_notificationTitle;
std::string g_notificationMessage;

// ==================== Forward Declarations ====================
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CleanupRenderTarget();
void CreateRenderTarget();
void ApplyTheme();
void RenderUI();
void AutoSave();
void RefreshList();
void ShowNotification(const char* title, const char* message);
void OpenAddDialog();
void OpenEditDialog(const Account& acc);
void SaveAccount();
void CopyToClipboard(const std::string& text);
void SmartClearClipboard();

bool g_alwaysOnTop = false;

void ToggleAlwaysOnTop() {
    g_alwaysOnTop = !g_alwaysOnTop;
    if (g_alwaysOnTop) {
        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else {
        SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

// ==================== DirectX Helpers ====================
void CleanupRenderTarget() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (backBuffer) {
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
        backBuffer->Release();
    }
}

// ==================== Tray Icon ====================
void AddTrayIcon() {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    
    g_nid.hIcon = (HICON)LoadImageW(nullptr, L"app.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION); // Fallback
    }

    wcscpy_s(g_nid.szTip, L"PassDrop");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void ShowTrayMenu() {
    POINT pt; GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SHOW, L"Show");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    SetForegroundWindow(g_hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hWnd, nullptr);
    DestroyMenu(hMenu);
}

// ==================== Helpers ====================
std::string GetDefaultVaultPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path = exePath;
    path = path.substr(0, path.find_last_of(L'\\'));
    path += L"\\default.pvault";
    return std::string(path.begin(), path.end());
}

void AutoSave() {
    if (g_currentFile.empty()) g_currentFile = GetDefaultVaultPath();
    g_dataStore.saveToFile(std::wstring(g_currentFile.begin(), g_currentFile.end()));
}

std::string SimpleHash(const std::string& str) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(str));
}

void SmartClearClipboard() {
    if (g_lastCopiedHash.empty()) return;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - g_lastCopyTime).count();
    if (elapsed < CLIPBOARD_TIMEOUT_SEC) return;

    if (OpenClipboard(nullptr)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* wstr = (wchar_t*)GlobalLock(hData);
            if (wstr) {
                std::wstring wcontent(wstr);
                std::string content(wcontent.begin(), wcontent.end());
                if (SimpleHash(content) == g_lastCopiedHash) EmptyClipboard();
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
    g_lastCopiedHash.clear();
}

void CopyToClipboard(const std::string& text) {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
        if (h) {
            wchar_t* wstr = (wchar_t*)GlobalLock(h);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wstr, wlen);
            GlobalUnlock(h);
            SetClipboardData(CF_UNICODETEXT, h);
        }
        CloseClipboard();
        g_lastCopiedHash = SimpleHash(text);
        g_lastCopyTime = std::chrono::steady_clock::now();
    }
}

void RefreshList() {
    if (g_search[0]) {
        std::wstring query(g_search, g_search + strlen(g_search));
        g_accounts = g_dataStore.searchAccounts(query);
    }
    else {
        g_accounts = g_dataStore.getAllAccounts();
    }
}

int CalculatePasswordStrength(const char* pass) {
    std::string p(pass);
    if (p.empty()) return 0;
    int score = 0;
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSpecial = false;
    for (char c : p) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (islower(uc)) hasLower = true;
        else if (isupper(uc)) hasUpper = true;
        else if (isdigit(uc)) hasDigit = true;
        else hasSpecial = true;
    }
    if (hasLower) score++;
    if (hasUpper) score++;
    if (hasDigit) score++;
    if (hasSpecial) score++;
    if (p.length() >= 12) score++;
    return score > 5 ? 5 : score;
}

// ==================== CSV Export/Import ====================
// Actual export logic (without warning)
void DoExportToCSV() {
    OPENFILENAMEA ofn = { sizeof(ofn) };
    char path[MAX_PATH] = "";
    ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "csv";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameA(&ofn)) return;

    std::ofstream file(path);
    if (!file.is_open()) return;

    file << "Title,URL,Login,Password,Extra1Label,Extra1Data,Extra2Label,Extra2Data,Icon,Color\n";
    for (auto& acc : g_dataStore.getAllAccounts()) {
        auto decLogin = CryptoService::getInstance().decrypt(acc.loginEnc);
        auto decPass = CryptoService::getInstance().decrypt(acc.passwordEnc);
        auto decE1 = CryptoService::getInstance().decrypt(acc.extra1DataEnc);
        auto decE2 = CryptoService::getInstance().decrypt(acc.extra2DataEnc);

        std::string login(decLogin.begin(), decLogin.end());
        std::string pass(decPass.begin(), decPass.end());
        std::string e1(decE1.begin(), decE1.end());
        std::string e2(decE2.begin(), decE2.end());

        file << "\"" << std::string(acc.title.begin(), acc.title.end()) << "\",";
        file << "\"" << std::string(acc.url.begin(), acc.url.end()) << "\",";
        file << "\"" << login << "\",";
        file << "\"" << pass << "\",";
        file << "\"" << std::string(acc.extra1Label.begin(), acc.extra1Label.end()) << "\",";
        file << "\"" << e1 << "\",";
        file << "\"" << std::string(acc.extra2Label.begin(), acc.extra2Label.end()) << "\",";
        file << "\"" << e2 << "\",";
        file << "\"" << std::string(acc.icon.begin(), acc.icon.end()) << "\",";
        file << acc.colorIndex << "\n";
    }
    file.close();

    // Success popup
    g_showAbout = false; // Reuse About popup or create new one
    ShowNotification("Export", "Export completed successfully!");
}

// Entry point with warning
void ExportToCSV() {
    g_exportPending = true;
    g_showExportWarning = true;
}

void DoImportFromCSV() {
    OPENFILENAMEA ofn = { sizeof(ofn) };
    char path[MAX_PATH] = "";
    ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    if (!GetOpenFileNameA(&ofn)) return;

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line); // Skip header

    int imported = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::vector<std::string> parts;
        bool inQuotes = false;
        std::string current;
        for (char c : line) {
            if (c == '"') { inQuotes = !inQuotes; continue; }
            if (c == ',' && !inQuotes) { parts.push_back(current); current.clear(); continue; }
            current += c;
        }
        parts.push_back(current);

        // Need exactly 10 columns
        if (parts.size() < 10) continue;

        // Remove surrounding quotes from each part
        for (auto& p : parts) {
            if (p.size() >= 2 && p.front() == '"' && p.back() == '"') {
                p = p.substr(1, p.size() - 2);
            }
        }

        Account acc;
        acc.title = std::wstring(parts[0].begin(), parts[0].end());
        acc.url = std::wstring(parts[1].begin(), parts[1].end());
        acc.extra1Label = std::wstring(parts[4].begin(), parts[4].end());
        acc.extra2Label = std::wstring(parts[6].begin(), parts[6].end());
        acc.icon = std::wstring(parts[8].begin(), parts[8].end());
        acc.colorIndex = atoi(parts[9].c_str());

        acc.loginEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(parts[2].begin(), parts[2].end()));
        acc.passwordEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(parts[3].begin(), parts[3].end()));
        acc.extra1DataEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(parts[5].begin(), parts[5].end()));
        acc.extra2DataEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(parts[7].begin(), parts[7].end()));

        g_dataStore.addAccount(acc);
        imported++;
    }
    RefreshList();
    AutoSave();
    ShowNotification("Import", ("Imported " + std::to_string(imported) + " accounts.").c_str());
}

// ==================== File Dialogs ====================
void SaveVaultAs() {
    OPENFILENAMEA ofn = { sizeof(ofn) };
    char path[MAX_PATH] = "";
    ofn.lpstrFilter = "PassDrop Vault (*.pvault)\0*.pvault\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "pvault";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameA(&ofn)) { g_currentFile = path; AutoSave(); }
}

void OpenVault() {
    OPENFILENAMEA ofn = { sizeof(ofn) };
    char path[MAX_PATH] = "";
    ofn.lpstrFilter = "PassDrop Vault (*.pvault)\0*.pvault\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        g_currentFile = path;
        if (g_dataStore.loadFromFile(std::wstring(path, path + strlen(path)))) {
            RefreshList();
        }
    }
}

void NewVault() {
    g_showNewConfirm = true;
}

// ==================== Dialogs ====================
void OpenAddDialog() {
    g_editAccount = Account();
    g_editAccount.extra1Label = L"Note";
    g_editAccount.extra2Label = L"PIN";
    g_isEdit = false;
    memset(&g_buf, 0, sizeof(g_buf));
    strcpy_s(g_buf.e1label, sizeof(g_buf.e1label), "Note");
    strcpy_s(g_buf.e2label, sizeof(g_buf.e2label), "PIN");
    g_buf.iconIndex = 0;
    g_buf.colorIndex = 0;
    g_showAddEdit = true;
}

void OpenEditDialog(const Account& acc) {
    g_editAccount = acc;
    g_isEdit = true;
    memset(&g_buf, 0, sizeof(g_buf));

    std::string title(acc.title.begin(), acc.title.end());
    std::string url(acc.url.begin(), acc.url.end());
    std::string e1label(acc.extra1Label.begin(), acc.extra1Label.end());
    std::string e2label(acc.extra2Label.begin(), acc.extra2Label.end());

    strcpy_s(g_buf.title, sizeof(g_buf.title), title.c_str());
    strcpy_s(g_buf.url, sizeof(g_buf.url), url.c_str());
    strcpy_s(g_buf.e1label, sizeof(g_buf.e1label), e1label.c_str());
    strcpy_s(g_buf.e2label, sizeof(g_buf.e2label), e2label.c_str());

    if (!acc.loginEnc.empty()) {
        auto dec = CryptoService::getInstance().decrypt(acc.loginEnc);
        std::string s(dec.begin(), dec.end());
        strcpy_s(g_buf.login, sizeof(g_buf.login), s.c_str());
    }
    if (!acc.passwordEnc.empty()) {
        auto dec = CryptoService::getInstance().decrypt(acc.passwordEnc);
        std::string s(dec.begin(), dec.end());
        strcpy_s(g_buf.pass, sizeof(g_buf.pass), s.c_str());
    }
    if (!acc.extra1DataEnc.empty()) {
        auto dec = CryptoService::getInstance().decrypt(acc.extra1DataEnc);
        std::string s(dec.begin(), dec.end());
        strcpy_s(g_buf.e1data, sizeof(g_buf.e1data), s.c_str());
    }
    if (!acc.extra2DataEnc.empty()) {
        auto dec = CryptoService::getInstance().decrypt(acc.extra2DataEnc);
        std::string s(dec.begin(), dec.end());
        strcpy_s(g_buf.e2data, sizeof(g_buf.e2data), s.c_str());
    }

    for (int i = 0; i < ICON_COUNT; ++i)
        if (acc.icon == ICONS[i]) { g_buf.iconIndex = i; break; }
    g_buf.colorIndex = acc.colorIndex;
    g_showAddEdit = true;
}

void SaveAccount() {
    g_editAccount.title = std::wstring(g_buf.title, g_buf.title + strlen(g_buf.title));
    g_editAccount.url = std::wstring(g_buf.url, g_buf.url + strlen(g_buf.url));
    g_editAccount.extra1Label = std::wstring(g_buf.e1label, g_buf.e1label + strlen(g_buf.e1label));
    g_editAccount.extra2Label = std::wstring(g_buf.e2label, g_buf.e2label + strlen(g_buf.e2label));
    g_editAccount.icon = ICONS[g_buf.iconIndex];
    g_editAccount.colorIndex = g_buf.colorIndex;

    std::string l(g_buf.login), p(g_buf.pass), e1(g_buf.e1data), e2(g_buf.e2data);
    g_editAccount.loginEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(l.begin(), l.end()));
    g_editAccount.passwordEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(p.begin(), p.end()));
    g_editAccount.extra1DataEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(e1.begin(), e1.end()));
    g_editAccount.extra2DataEnc = CryptoService::getInstance().encrypt(std::vector<uint8_t>(e2.begin(), e2.end()));

    if (g_isEdit) g_dataStore.updateAccount(g_editAccount);
    else g_dataStore.addAccount(g_editAccount);

    SecureZeroMemory(g_buf.pass, sizeof(g_buf.pass));
    SecureZeroMemory(g_buf.login, sizeof(g_buf.login));
    AutoSave();
}

// ==================== Window Procedure ====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            ShowWindow(hWnd, SW_HIDE);  // Hide to tray instead of taskbar
            return 0;
        }
        if (g_device && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_NCCALCSIZE: return 0;
    case WM_NCHITTEST: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);
        RECT rc; GetClientRect(hWnd, &rc);
        const int border = 5;
        if (pt.y < border) {
            if (pt.x < border) return HTTOPLEFT;
            if (pt.x > rc.right - border) return HTTOPRIGHT;
            return HTTOP;
        }
        if (pt.y > rc.bottom - border) {
            if (pt.x < border) return HTBOTTOMLEFT;
            if (pt.x > rc.right - border) return HTBOTTOMRIGHT;
            return HTBOTTOM;
        }
        if (pt.x < border) return HTLEFT;
        if (pt.x > rc.right - border) return HTRIGHT;
        if (pt.y < 40) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_RBUTTONUP) ShowTrayMenu();
        if (LOWORD(lParam) == WM_LBUTTONUP) ShowWindow(hWnd, SW_SHOW);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_SHOW) ShowWindow(hWnd, SW_SHOW);
        if (LOWORD(wParam) == ID_TRAY_EXIT) { g_running = false; DestroyWindow(hWnd); }
        return 0;
    case WM_CLOSE:
        if (g_dataStore.isModified() && !g_currentFile.empty()) {
            int res = MessageBoxW(hWnd, L"Save changes before exit?", L"PassDrop", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (res == IDYES) AutoSave();
            if (res == IDCANCEL) return 0;
        }
        SmartClearClipboard();
        RemoveTrayIcon();
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ==================== Theme ====================
void ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0;
    style.FrameRounding = 4;
    style.GrabRounding = 4;
    if (g_darkTheme) {
        ImGui::StyleColorsDark();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    }
    else {
        ImGui::StyleColorsLight();
    }
}

void RenderPasswordStrength(const char* pass) {
    int strength = CalculatePasswordStrength(pass);
    const char* labels[] = { "Very Weak", "Weak", "Fair", "Good", "Strong", "Very Strong" };
    ImVec4 colors[] = {
        ImVec4(0.8f, 0.2f, 0.2f, 1.0f), ImVec4(1.0f, 0.5f, 0.2f, 1.0f),
        ImVec4(1.0f, 0.8f, 0.2f, 1.0f), ImVec4(0.5f, 0.8f, 0.2f, 1.0f),
        ImVec4(0.2f, 0.8f, 0.2f, 1.0f), ImVec4(0.2f, 0.6f, 0.2f, 1.0f),
    };
    ImGui::Text("Strength: "); ImGui::SameLine();
    ImGui::TextColored(colors[strength], "%s", labels[strength]);
    ImGui::ProgressBar((float)strength / 5.0f, ImVec2(-1, 0), "");
}

void TextWithHighlight(const char* text, const char* search) {
    if (!search || !search[0]) { ImGui::TextUnformatted(text); return; }
    std::string t(text), s(search);
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    size_t pos = t.find(s);
    if (pos == std::string::npos) { ImGui::TextUnformatted(text); return; }
    ImGui::TextUnformatted(text, text + pos);
    ImGui::SameLine(0, 0);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.*s", (int)s.length(), text + pos);
    ImGui::SameLine(0, 0);
    ImGui::TextUnformatted(text + pos + s.length());
}

// ==================== Main UI ====================
void RenderUI() {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##Main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(2);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) NewVault();
            if (ImGui::MenuItem("Open...")) OpenVault();
            if (ImGui::MenuItem("Save As...")) SaveVaultAs();
            ImGui::Separator();
            if (ImGui::MenuItem("Import from CSV...")) DoImportFromCSV();
            if (ImGui::MenuItem("Export to CSV...")) ExportToCSV();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) { g_running = false; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Extra 1", nullptr, &g_showExtra1);
            ImGui::MenuItem("Show Extra 2", nullptr, &g_showExtra2);
            ImGui::Separator();
            if (ImGui::MenuItem("Always on Top", nullptr, g_alwaysOnTop)) {
                ToggleAlwaysOnTop();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Dark Theme", nullptr, g_darkTheme)) { g_darkTheme = true; ApplyTheme(); }
            if (ImGui::MenuItem("Light Theme", nullptr, !g_darkTheme)) { g_darkTheme = false; ApplyTheme(); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) g_showAbout = true;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Title bar
    ImGui::TextUnformatted("PassDrop");
    ImGui::SameLine();
    ImGui::SetCursorPosX(io.DisplaySize.x - 160);

    // Always on Top toggle button
    if (ImGui::Button(g_alwaysOnTop ? "▲" : "△", ImVec2(30, 25))) {
        ToggleAlwaysOnTop();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(g_alwaysOnTop ? "Always on Top (ON)" : "Always on Top (OFF)");
    }

    ImGui::SameLine();
    if (ImGui::Button("_##Min", ImVec2(30, 25))) ShowWindow(g_hWnd, SW_MINIMIZE);
    ImGui::SameLine();
    if (ImGui::Button(g_maximized ? "[ ]##Max" : "[_]##Max", ImVec2(30, 25))) {
        if (g_maximized) {
            // Restore
            SetWindowLongPtr(g_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
            SetWindowPos(g_hWnd, nullptr, g_prevPlacement.rcNormalPosition.left,
                g_prevPlacement.rcNormalPosition.top,
                g_prevPlacement.rcNormalPosition.right - g_prevPlacement.rcNormalPosition.left,
                g_prevPlacement.rcNormalPosition.bottom - g_prevPlacement.rcNormalPosition.top,
                SWP_FRAMECHANGED | SWP_NOZORDER);
            g_maximized = false;
        }
        else {
            // Maximize
            GetWindowPlacement(g_hWnd, &g_prevPlacement);
            HMONITOR hMonitor = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(hMonitor, &mi);
            SetWindowLongPtr(g_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_THICKFRAME);
            SetWindowPos(g_hWnd, nullptr, mi.rcWork.left, mi.rcWork.top,
                mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top,
                SWP_FRAMECHANGED | SWP_NOZORDER);
            g_maximized = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("X##Close", ImVec2(30, 25))) SendMessage(g_hWnd, WM_CLOSE, 0, 0);
    ImGui::Separator();

    // Toolbar
    if (ImGui::Button("+ Add Entry", ImVec2(110, 30))) OpenAddDialog();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::InputTextWithHint("##search", "Search...", g_search, sizeof(g_search),
        ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars)) RefreshList();
    if (ImGui::Button("New", ImVec2(70, 30))) NewVault();
    ImGui::SameLine();
    if (ImGui::Button("Open", ImVec2(70, 30))) OpenVault();
    ImGui::SameLine();
    if (ImGui::Button("Save as", ImVec2(80, 30))) SaveVaultAs();
    ImGui::SameLine();
    if (ImGui::Button("Import from CSV", ImVec2(140, 30))) DoImportFromCSV();
    ImGui::SameLine();
    if (ImGui::Button("Export to CSV", ImVec2(120, 30))) ExportToCSV();
    ImGui::SameLine();
    if (ImGui::Button("About", ImVec2(70, 30))) g_showAbout = true;

    // Table
    int colCount = 4 + (g_showExtra1 ? 1 : 0) + (g_showExtra2 ? 1 : 0);
    ImVec2 tableSize(0, io.DisplaySize.y - 130);
    if (ImGui::BeginTable("##table", colCount,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, tableSize)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 200);
        ImGui::TableSetupColumn("Login", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Password", ImGuiTableColumnFlags_WidthFixed, 120);
        int nextCol = 3;
        if (g_showExtra1) ImGui::TableSetupColumn("Extra 1", ImGuiTableColumnFlags_WidthFixed, 120);
        if (g_showExtra2) ImGui::TableSetupColumn("Extra 2", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 200);
        ImGui::TableHeadersRow();

        for (auto& acc : g_accounts) {
            ImGui::PushID(acc.id);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImVec4 color = COLORS[acc.colorIndex % COLOR_COUNT];
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            std::string title = std::string(acc.icon.begin(), acc.icon.end()) + " " + std::string(acc.title.begin(), acc.title.end());
            TextWithHighlight(title.c_str(), g_search);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !acc.url.empty())
                ShellExecuteW(nullptr, L"open", acc.url.c_str(), nullptr, nullptr, SW_SHOW);

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button(("Copy##Login" + std::to_string(acc.id)).c_str(), ImVec2(100, 0))) {
                auto dec = CryptoService::getInstance().decrypt(acc.loginEnc);
                CopyToClipboard(std::string(dec.begin(), dec.end()));
            }

            ImGui::TableSetColumnIndex(2);
            if (g_showPassMap[acc.id]) {
                auto dec = CryptoService::getInstance().decrypt(acc.passwordEnc);
                ImGui::TextUnformatted(std::string(dec.begin(), dec.end()).c_str());
            }
            else {
                ImGui::TextUnformatted("••••••••");
            }
            ImGui::SameLine();
            if (ImGui::Button(g_showPassMap[acc.id] ? "Hide" : "Show", ImVec2(40, 0)))
                g_showPassMap[acc.id] = !g_showPassMap[acc.id];
            ImGui::SameLine();
            if (ImGui::Button(("Copy##Pass" + std::to_string(acc.id)).c_str(), ImVec2(40, 0))) {
                auto dec = CryptoService::getInstance().decrypt(acc.passwordEnc);
                CopyToClipboard(std::string(dec.begin(), dec.end()));
            }

            int colIdx = 3;
            if (g_showExtra1) {
                ImGui::TableSetColumnIndex(colIdx++);
                if (!acc.extra1DataEnc.empty() && ImGui::Button(("Copy##E1" + std::to_string(acc.id)).c_str(), ImVec2(100, 0))) {
                    auto dec = CryptoService::getInstance().decrypt(acc.extra1DataEnc);
                    CopyToClipboard(std::string(dec.begin(), dec.end()));
                }
            }
            if (g_showExtra2) {
                ImGui::TableSetColumnIndex(colIdx++);
                if (!acc.extra2DataEnc.empty() && ImGui::Button(("Copy##E2" + std::to_string(acc.id)).c_str(), ImVec2(100, 0))) {
                    auto dec = CryptoService::getInstance().decrypt(acc.extra2DataEnc);
                    CopyToClipboard(std::string(dec.begin(), dec.end()));
                }
            }

            ImGui::TableSetColumnIndex(colIdx);
            if (ImGui::Button(("Edit##" + std::to_string(acc.id)).c_str(), ImVec2(50, 0))) OpenEditDialog(acc);
            ImGui::SameLine();
            if (ImGui::Button(("Del##" + std::to_string(acc.id)).c_str(), ImVec2(50, 0))) {
                g_deleteId = acc.id;
                g_showDeleteConfirm = true;
            }
            if (!acc.url.empty()) {
                ImGui::SameLine();
                if (ImGui::Button(("URL##" + std::to_string(acc.id)).c_str(), ImVec2(50, 0)))
                    ShellExecuteW(nullptr, L"open", acc.url.c_str(), nullptr, nullptr, SW_SHOW);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // About popup
    if (g_showAbout) {
        ImGui::OpenPopup("About");
        g_showAbout = false;
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("PassDrop v1.0");
        ImGui::Separator();
        ImGui::TextUnformatted("Modern password manager");
        ImGui::TextUnformatted("Developer: Maksutov Vladislav Vitalievich (Pichan)");
        // Social links (clickable)
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Telegram: @PichanDev");
        if (ImGui::IsItemClicked()) ShellExecuteA(nullptr, "open", "https://t.me/PichanDev", nullptr, nullptr, SW_SHOW);

        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "X(Twitter): x.com/PichanTsu");
        if (ImGui::IsItemClicked()) ShellExecuteA(nullptr, "open", "https://x.com/PichanTsu", nullptr, nullptr, SW_SHOW);
        ImGui::TextUnformatted("mail: vmaksutcf@mail.ru");
        ImGui::TextUnformatted("License: GNU GPL v3");
        if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Delete confirm popup
    if (g_showDeleteConfirm) {
        ImGui::OpenPopup("Delete?");
        g_showDeleteConfirm = false;
    }
    if (ImGui::BeginPopupModal("Delete?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Delete this entry?");
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
            g_dataStore.deleteAccount(g_deleteId);
            RefreshList();
            AutoSave();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // New Vault confirm popup
    if (g_showNewConfirm) {
        ImGui::OpenPopup("New Vault");
        g_showNewConfirm = false;
    }
    if (ImGui::BeginPopupModal("New Vault", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Create a new vault?");
        ImGui::Spacing();

        // Red warning text
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("WARNING: Current data will be lost if not saved!");
        ImGui::PopStyleColor();

        ImGui::TextWrapped("Use 'Save As' first to keep your old passwords.");
        ImGui::Spacing();

        if (ImGui::Button("Yes", ImVec2(80, 0))) {
            g_dataStore = DataStore();
            g_currentFile.clear();
            g_accounts.clear();
            g_search[0] = '\0';
            RefreshList();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Add/Edit popup
    if (g_showAddEdit) {
        ImGui::OpenPopup(g_isEdit ? "Edit Account" : "Add Account");
        g_showAddEdit = false;
    }
    if (ImGui::BeginPopupModal(g_isEdit ? "Edit Account" : "Add Account", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Title", g_buf.title, sizeof(g_buf.title),
            ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);
        ImGui::InputText("URL", g_buf.url, sizeof(g_buf.url),
            ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);
        ImGui::InputText("Login", g_buf.login, sizeof(g_buf.login),
            ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);
        ImGui::InputText("Password", g_buf.pass, sizeof(g_buf.pass),
            ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);

        RenderPasswordStrength(g_buf.pass);

       
        if (ImGui::CollapsingHeader("Password Generator")) {
            ImGui::SliderInt("Length", &g_pwdLength, 4, 32);
            ImGui::Checkbox("Uppercase", &g_pwdUpper);
            ImGui::Checkbox("Lowercase", &g_pwdLower);
            ImGui::Checkbox("Digits", &g_pwdDigits);
            ImGui::Checkbox("Special", &g_pwdSpecial);

            if (ImGui::Button("Generate", ImVec2(100, 25))) {
                static bool seeded = false;
                if (!seeded) { srand((unsigned)time(nullptr)); seeded = true; }

                std::wstring chars;
                if (g_pwdUpper) chars += L"ABCDEFGHJKLMNPQRSTUVWXYZ";
                if (g_pwdLower) chars += L"abcdefghijkmnpqrstuvwxyz";
                if (g_pwdDigits) chars += L"23456789";
                if (g_pwdSpecial) chars += L"!@#$%^&*_-+=?";
                if (chars.empty()) chars = L"abcdefghijkmnpqrstuvwxyz23456789";

                std::wstring pwd;
                for (int i = 0; i < g_pwdLength; ++i)
                    pwd += chars[rand() % chars.length()];

                int len = WideCharToMultiByte(CP_UTF8, 0, pwd.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string pwdUtf8(len, 0);
                WideCharToMultiByte(CP_UTF8, 0, pwd.c_str(), -1, &pwdUtf8[0], len, nullptr, nullptr);

                strcpy_s(g_buf.pass, sizeof(g_buf.pass), pwdUtf8.c_str());
            }
        }
        // ==========================================

        ImGui::Separator();

        // Icon selector
        if (ImGui::BeginCombo("Icon", "Select...")) {
            for (int i = 0; i < ICON_COUNT; ++i) {
                std::string iconStr(ICONS[i], ICONS[i] + wcslen(ICONS[i]));
                if (ImGui::Selectable(iconStr.c_str(), g_buf.iconIndex == i)) {
                    g_buf.iconIndex = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::Text("%s", std::string(ICONS[g_buf.iconIndex], ICONS[g_buf.iconIndex] + wcslen(ICONS[g_buf.iconIndex])).c_str());

        // Color selector
        if (ImGui::BeginCombo("Color", COLOR_NAMES[g_buf.colorIndex])) {
            for (int i = 0; i < COLOR_COUNT; ++i)
                if (ImGui::Selectable(COLOR_NAMES[i], g_buf.colorIndex == i))
                    g_buf.colorIndex = i;
            ImGui::EndCombo();
        }

        ImGui::Separator();
        ImGui::InputText("Extra 1 Label", g_buf.e1label, sizeof(g_buf.e1label),
            ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);
        ImGui::InputText("Extra 1 Data", g_buf.e1data, sizeof(g_buf.e1data),
            ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);
        ImGui::InputText("Extra 2 Label", g_buf.e2label, sizeof(g_buf.e2label),
            ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);
        ImGui::InputText("Extra 2 Data", g_buf.e2data, sizeof(g_buf.e2data),
            ImGuiInputTextFlags_CallbackCharFilter, FilterSupportedChars);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(100, 30))) {
            SaveAccount();
            RefreshList();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            SecureZeroMemory(g_buf.pass, sizeof(g_buf.pass));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Export Warning popup
    if (g_showExportWarning) {
        ImGui::OpenPopup("Security Warning");
        g_showExportWarning = false;
    }
    if (ImGui::BeginPopupModal("Security Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("WARNING: CSV export is NOT encrypted!");
        ImGui::PopStyleColor();
        ImGui::TextWrapped("Anyone can read this file. Continue?");
        ImGui::Spacing();

        if (ImGui::Button("Yes, Export", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
            if (g_exportPending) {
                DoExportToCSV(); // Actual export function
                g_exportPending = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
            g_exportPending = false;
        }
        ImGui::EndPopup();
    }

    // Import Warning popup
    if (g_showImportWarning) {
        ImGui::OpenPopup("Import Warning");
        g_showImportWarning = false;
    }
    if (ImGui::BeginPopupModal("Import Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
        ImGui::TextWrapped("Warning: Importing will add accounts from CSV.");
        ImGui::PopStyleColor();
        ImGui::TextWrapped("Make sure the CSV file is from a trusted source.");
        ImGui::Spacing();

        if (ImGui::Button("Continue", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
            if (g_importPending) {
                DoImportFromCSV();
                g_importPending = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
            g_importPending = false;
        }
        ImGui::EndPopup();
    }

    // Notification popup
    if (g_showNotification) {
        ImGui::OpenPopup(g_notificationTitle.c_str());
        g_showNotification = false;
    }
    if (ImGui::BeginPopupModal(g_notificationTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", g_notificationMessage.c_str());
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

// ==================== Entry Point ====================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    // Register window class 
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);       
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);     
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PassDrop";
    RegisterClassExW(&wc);

    // Create window
    g_hWnd = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_LAYERED, wc.lpszClassName, L"PassDrop",
        WS_POPUP | WS_VISIBLE,
        100, 100, 1000, 700, nullptr, nullptr, hInst, nullptr);

    
    HICON hIcon = (HICON)LoadImageW(nullptr, L"app.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (hIcon) {
        SendMessage(g_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
    // ==============================================

    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(g_hWnd, &margins);
    AddTrayIcon();

    SetLayeredWindowAttributes(g_hWnd, 0, 255, LWA_ALPHA);

    // DirectX 11
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_swapChain, &g_device, nullptr, &g_context);
    CreateRenderTarget();

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Load fonts: English + Russian + Japanese using system font
    io.Fonts->Clear();
    ImFontConfig cfg;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;

    // Build ranges
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());   // English
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());  // Russian
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());  // Japanese

    static ImVector<ImWchar> ranges;
    builder.BuildRanges(&ranges);

    ImFont* font = nullptr;

    // Try Japanese fonts first (they support Latin and Cyrillic too)
    font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\YuGothM.ttc", 18.0f, &cfg, ranges.Data);
    if (!font) {
        font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", 18.0f, &cfg, ranges.Data);
    }
    if (!font) {
        font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f, &cfg, ranges.Data);
    }
    if (!font) {
        io.Fonts->AddFontDefault();
    }
    if (font) {
        io.FontDefault = font;
    }

    ApplyTheme();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_device, g_context);

    // Load default vault
    std::string defaultPath = GetDefaultVaultPath();
    if (g_dataStore.loadFromFile(std::wstring(defaultPath.begin(), defaultPath.end()))) {
        g_currentFile = defaultPath;
        RefreshList();
    }

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    // Main loop
    MSG msg = {};
    while (g_running && msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        if (!g_rtv) continue;
        if (!g_lastCopiedHash.empty()) SmartClearClipboard();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swapChain->Present(1, 0);
    }

    // Cleanup
    CleanupRenderTarget();
    if (g_swapChain) g_swapChain->Release();
    if (g_context) g_context->Release();
    if (g_device) g_device->Release();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

void ShowNotification(const char* title, const char* message) {
    g_notificationTitle = title;
    g_notificationMessage = message;
    g_showNotification = true;
}