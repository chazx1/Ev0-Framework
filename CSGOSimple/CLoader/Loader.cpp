#include "Loader.hpp"
#include "md5.h"
#include <fstream>

bool& CLoader::debugger_detected =  *new bool(false);

inline size_t writeCallbackFunc(void *contents, size_t size, size_t nmemb, void *userp) {
	MUT("CALLBACK_FUNC");
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
	END();
}

auto CLoader::getHWID()->std::string {
	return "OWNED";
	/*int nSize = VMProtectGetCurrentHWID(NULL, 0);
	char *p = new char[nSize];
	VMProtectGetCurrentHWID(p, nSize);
	return p;*/
}

auto CLoader::get(std::string url)->std::string {
	MUT("GET");
	if (debugger_detected)
		return "";
	static auto CheckCert = [](struct curl_certinfo* certinfo) {
		if (certinfo) {
			if (certinfo) {
				int i;
				for (i = 0; i < certinfo->num_of_certs; i++) {
					struct curl_slist *slist;
					std::string result = "";
					for (slist = certinfo->certinfo[i]; slist; slist = slist->next) {
						//printf("%s\n", slist->data);
						result += md5(slist->data);
						//
					}
					result = md5(result);
					//MessageBox(0, result.c_str(), 0, 0);
					return result.compare(XorStr("1a5127a4c77f3b198858fa2d85331bbe")) == 0;
				}
			}
		}
		return false;
	};
	//win32_copy_to_clipboardW(url.c_str());
	std::cout << "downloadin" << std::endl;
	CURL *curl;
	CURLcode res;
	std::string readBuffer;
	//std::string readBuffer(1000000,'\0');
	curl = curl_easy_init();
	if (!curl) {
		curl_easy_cleanup(curl);
		return "";
	}
	std::cout << "curl init ok" << std::endl;
	//url = curl_easy_escape(curl, url.c_str(), url.size());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_CERTINFO, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallbackFunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	res = curl_easy_perform(curl);
	struct curl_certinfo *certinfo;
	curl_easy_getinfo(curl, CURLINFO_CERTINFO, &certinfo);
	//if (!CheckCert(certinfo))
	//	return "";
	curl_easy_cleanup(curl);
	if (res != CURLcode::CURLE_OK)
		return get(url);
	if (readBuffer[0] == -17)
	{
		readBuffer = readBuffer.substr(3, readBuffer.length() - 3);
	}
	return readBuffer;
	END();
}


auto CLoader::CheckIsUser()->bool
{
	return true;
	static auto GetRegKey = [](std::string key) {
		HKEY rKey;
		char Path[260] = { 0 };
		DWORD RegetPath = sizeof(Path);
		RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\FLEXHACK", NULL, KEY_QUERY_VALUE, &rKey);
		RegQueryValueExA(rKey, key.c_str(), NULL, NULL, (LPBYTE)&Path, &RegetPath);
		return std::string(Path);
	};
	std::string log = GetRegKey("Login");
	std::string hwid = md5(getHWID()); 
	std::string result = get(XorStr("https://loader.flexhack.ru/licensecheck?binding=") + hwid + "&login=" + log);
	auto r = result.compare(md5(log + hwid + XorStr("NuB50bdQeFzFbKZAkhLp"))) == 0;
	return r && !debugger_detected;
};
