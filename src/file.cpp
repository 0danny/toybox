#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <string>

std::filesystem::path OpenFileDialog(const std::wstring& extension)
{
	OPENFILENAMEW ofn = { 0 };
	wchar_t szFile[MAX_PATH] = { 0 };

	// Create "*.txt", "*.png", etc.
	std::wstring pattern = L"*" + extension;

	// Build filter:
	// "TXT Files\0*.txt\0\0"
	std::wstring filter = extension.substr(1) + L" Files";
	filter.push_back(L'\0');
	filter += pattern;
	filter.push_back(L'\0');
	filter.push_back(L'\0');

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filter.c_str();
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameW(&ofn))
		return std::filesystem::path(szFile);

	return {};
}