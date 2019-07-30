#pragma once
#include <iostream>
#include <string>
#include <Windows.h>
#include "../singleton.hpp"
#include "../XORSTR.h"
#include "md5.h"
#include "../VMP.hpp"

#include <curl/curl.h>
#pragma comment(lib, "libcurl_a.lib")

class CLoader : public Singleton<CLoader> {
	static std::string key;
	static std::string dll;
	auto get(std::string url)->std::string;
public:
	auto getHWID()->std::string;
	auto CheckIsUser()->bool;
	static bool& debugger_detected;
};
