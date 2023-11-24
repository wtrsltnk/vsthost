#pragma once

#include <string>

std::string ConvertWideToBytes(const std::wstring& wstr);

std::wstring ConvertFromBytes(const std::string& str);

std::string ConvertWideToUtf8(const std::wstring& wstr);

std::wstring ConvertUtf8ToWide(const std::string& str);
