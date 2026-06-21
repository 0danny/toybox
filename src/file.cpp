#include <windows.h>
#include <commdlg.h>
#include <filesystem>

std::filesystem::path OpenFileDialog() {
    OPENFILENAMEW ofn = { 0 };
    wchar_t szFile[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        return std::filesystem::path(szFile);
    }
    else {
        return {};
    }
}