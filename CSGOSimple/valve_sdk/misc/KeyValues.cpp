#include "../sdk.hpp"


	
	
	void KeyValues::InitKetValues( KeyValues* pKeyValues , const char* name )
	{
		static auto key_values = reinterpret_cast<void(__thiscall*)(void*, const char*)>(Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 51 33 C0 C7 45 ? ? ? ? ? 56 8B F1 81 26 ? ? ? ? C6 46 03 00 89 46 10 89 46 18 89 46 14 89 46 1C 89 46 04 89 46 08 89 46 0C FF 15? ? ? ? 6A 01 FF 75 08 8D 4D FC 8B 10 51 8B C8 FF 52 24 8B 0E 33 4D FC 81 E1 ? ? ? ? 31 0E 88 46 03"));
		key_values(pKeyValues, name);
	}
	
	//PVOID KeyValues::operator new(size_t iAllocSize)
	//{
	//	typedef PVOID(__thiscall* oAllocKeyValuesMemory)(PVOID, size_t);
	//	return GetMethod<oAllocKeyValuesMemory>(CInterfaces::Get().KVS, 1)(CInterfaces::Get().KVS, iAllocSize);
	//}
	//void KeyValues::operator delete(PVOID pMem)
	//{
	//
	//	typedef void(__thiscall* oFreeKeyValuesMemory)(PVOID, PVOID);
	//	GetMethod<oFreeKeyValuesMemory>(CInterfaces::Get().KVS, 2)(CInterfaces::Get().KVS, pMem);
	//}
	bool KeyValues::LoadFromBuffer( KeyValues *pThis , const char *pszFirst , const char *pszSecond , PVOID pSomething , PVOID pAnother , PVOID pLast )
	{
		typedef bool( __thiscall *_LoadFromBuffer )( KeyValues* , const char* , const char* , PVOID , PVOID , PVOID );

		static _LoadFromBuffer LoadFromBufferFn = 0;
		static bool SearchFunction = false;

		if ( !SearchFunction )
		{
			DWORD dwFunctionAddress = (DWORD)Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89");

			if ( dwFunctionAddress )
			{
				LoadFromBufferFn = (_LoadFromBuffer)dwFunctionAddress;
				SearchFunction = true;
			}
		}

		if ( LoadFromBufferFn && SearchFunction )
		{
			return LoadFromBufferFn( pThis , pszFirst , pszSecond , pSomething , pAnother , pLast );
		}

		return false;
	}
	KeyValues::~KeyValues()
	{
		RemoveEverything();
	}
	void KeyValues::RemoveEverything()
	{
		KeyValues *dat;
		KeyValues *datNext = NULL;
		for (dat = m_pSub; dat != NULL; dat = datNext)
		{
			datNext = dat->m_pPeer;
			dat->m_pPeer = NULL;
			delete dat;
		}

		for (dat = m_pPeer; dat && dat != this; dat = datNext)
		{
			datNext = dat->m_pPeer;
			dat->m_pPeer = NULL;
			delete dat;
		}

		delete[] m_sValue;
		m_sValue = NULL;
		delete[] m_wsValue;
		m_wsValue = NULL;
	}
	void KeyValues::SetUint64(const char* keyName, int value, int value2) {
		auto key = FindKey(keyName, true);
		typedef void(__thiscall* fn)(void*, const char*, int, int);
		static auto key_values_set_uint64 = reinterpret_cast<fn>(Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 56 6A 01 FF 75 08"));
		key_values_set_uint64(this, keyName, value, value2);
	}
	void KeyValues::SetString(const char *value) {
		static auto key_values_set_string = reinterpret_cast<void(__thiscall*)(void*, const char*)>((Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC A1 ? ? ? ? 53 56 57 8B F9 8B 08 8B 01")));
		key_values_set_string(this, value);
	}
	void KeyValues::SetString(const char *keyName, const char *value) {
		auto key = FindKey(keyName, true);
		static auto key_values_set_string = reinterpret_cast<void(__thiscall*)(void*, const char*)>((Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC A1 ? ? ? ? 53 56 57 8B F9 8B 08 8B 01")));
		key_values_set_string(key, value);
	}
	int   KeyValues::GetInt(const char *keyName, int defaultValue) {
		auto key = FindKey(keyName, true);
		static auto key_values_get_int = reinterpret_cast<int(__thiscall*)(void*, const char*, int)>(Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 6A 00 FF 75 08 E8 ? ? ? ? 85 C0 74 45"));
		return key_values_get_int(this, keyName, defaultValue);
	}
	const char *KeyValues::GetString(const char *keyName, const char *defaultValue) {
		static auto key_values_get_string = reinterpret_cast<const char*(__thiscall*)(void*, const char*, const char*)>(Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 83 E4 C0 81 EC ? ? ? ? 53 8B 5D 08"));
		return key_values_get_string(this, keyName, defaultValue);
	}
	KeyValues *KeyValues::FindKey(const char *keyName, bool bCreate) {
		static auto key_values_find_key = reinterpret_cast<KeyValues*(__thiscall*)(void*, const char*, bool)>(Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 83 EC 1C 53 8B D9 85 DB"));
		return key_values_find_key(this, keyName, bCreate);
	}
	//void KeyValues::SetString(const char *keyName, const char *value)
	//{
	//	KeyValues *dat = FindKey(keyName, true);
//
	//	if (dat)
	//	{
	//		// delete the old value
	//		delete[] dat->m_sValue;
	//		// make sure we're not storing the WSTRING  - as we're converting over to STRING
	//		delete[] dat->m_wsValue;
	//		dat->m_wsValue = NULL;

	//		if (!value)
	//		{
	//			// ensure a valid value
	//			value = "";
	//		}

	//		// allocate memory for the new value and copy it in
	//		int len = strlen(value);
	//		dat->m_sValue = new char[len + 1];
	//		memcpy(dat->m_sValue, value, len + 1);

	//		dat->m_iDataType = TYPE_STRING;
	//	}
	//}
