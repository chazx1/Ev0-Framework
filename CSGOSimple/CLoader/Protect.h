#include <Windows.h>
#include <TlHelp32.h>
#include "../XorStr.h"
#include "Loader.hpp"
#include "../VMP.hpp"
#undef xor
BOOL IsVMware()
{
	MUT("IsVMware");
	BOOL bDetected = FALSE;

	__try
	{
		__asm
		{
			mov    ecx, 0Ah
			mov    eax, 'VMXh'
			mov    dx, 'VX'
			in    eax, dx
			cmp    ebx, 'VMXh'
			sete    al
			movzx   eax, al
			mov    bDetected, eax
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}
	END();
	return bDetected;
}

BOOL IsVM()
{
	HKEY hKey;
	int i;
	char szBuffer[64];
	const char *szProducts[] = { "*VMWARE*", "*VBOX*", "*VIRTUAL*" };

	DWORD dwSize = sizeof(szBuffer) - 1;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\Disk\\Enum", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, L"0", NULL, NULL, (unsigned char *)szBuffer, &dwSize) == ERROR_SUCCESS)
		{
			for (i = 0; i < _countof(szProducts); i++)
			{
				if (strstr(szBuffer, szProducts[i]))
				{
					RegCloseKey(hKey);
					return TRUE;
				}
			}
		}

		RegCloseKey(hKey);
	}
	return FALSE;
}

BOOL IsSandboxie()
{
	if (GetModuleHandle(L"SbieDll.dll") != NULL)
		return TRUE;


	return FALSE;
}

BOOL IsVirtualBox()
{
	BOOL bDetected = FALSE;

	if (LoadLibrary(L"VBoxHook.dll") != NULL)
		bDetected = TRUE;

	if (CreateFile(L"\\\\.\\VBoxMiniRdrDN", GENERIC_READ, \
		FILE_SHARE_READ, NULL, OPEN_EXISTING, \
		FILE_ATTRIBUTE_NORMAL, NULL) \
		!= INVALID_HANDLE_VALUE)
	{
		bDetected = TRUE;
	}

	return bDetected;
}

bool MemoryBreakpointDebuggerCheck()
{
	unsigned char *pMem = NULL;
	SYSTEM_INFO sysinfo = { 0 };
	DWORD OldProtect = 0;
	void *pAllocation = NULL;

	GetSystemInfo(&sysinfo);

	pAllocation = VirtualAlloc(NULL, sysinfo.dwPageSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	if (pAllocation == NULL)
		return false;

	pMem = (unsigned char*)pAllocation;
	*pMem = 0xc3;


	if (VirtualProtect(pAllocation, sysinfo.dwPageSize,
		PAGE_EXECUTE_READWRITE | PAGE_GUARD,
		&OldProtect) == 0)
	{
		return false;
	}

	__try
	{
		__asm
		{
			mov eax, pAllocation
			push MemBpBeingDebugged
			jmp eax
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		VirtualFree(pAllocation, NULL, MEM_RELEASE);
		return false;
	}

	__asm {MemBpBeingDebugged:}
	VirtualFree(pAllocation, NULL, MEM_RELEASE);
	return true;
}

inline bool Int2DCheck()
{
	__try
	{
		__asm
		{
			int 0x2d
			xor eax, eax
			add eax, 2
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

inline bool IsDbgPresentPrefixCheck()
{
	__try
	{
		__asm __emit 0xF3
		__asm __emit 0x64
		__asm __emit 0xF1
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

inline void PushPopSS()
{

	__asm
	{
		push ss
		pop ss
		mov eax, 0xC000C1EE
		xor edx, edx
	}

}

PVOID GetPEB()
{
#ifdef _WIN64
	return (PVOID)__readgsqword(0x0C * sizeof(PVOID));
#else
	return (PVOID)__readfsdword(0x0C * sizeof(PVOID));
#endif
}
#pragma warning(disable: 4996)
WORD GetVersionWord()
{
	OSVERSIONINFO verInfo = { sizeof(OSVERSIONINFO) };
	GetVersionEx(&verInfo);
	return MAKEWORD(verInfo.dwMinorVersion, verInfo.dwMajorVersion);
}

BOOL IsWin8OrHigher() { return GetVersionWord() >= _WIN32_WINNT_WIN8; }

BOOL IsVistaOrHigher() { return GetVersionWord() >= _WIN32_WINNT_VISTA; }

PVOID GetPEB64()
{
	PVOID pPeb = 0;
#ifndef _WIN64
	if (IsWin8OrHigher())
	{
		BOOL isWow64 = FALSE;
		typedef BOOL(WINAPI *pfnIsWow64Process)(HANDLE hProcess, PBOOL isWow64);
		pfnIsWow64Process fnIsWow64Process = (pfnIsWow64Process)
			GetProcAddress(GetModuleHandleA("Kernel32.dll"), "IsWow64Process");
		if (fnIsWow64Process(GetCurrentProcess(), &isWow64))
		{
			if (isWow64)
			{
				pPeb = (PVOID)__readfsdword(0x0C * sizeof(PVOID));
				pPeb = (PVOID)((PBYTE)pPeb + 0x1000);
			}
		}
	}
#endif
	return pPeb;
}

#define FLG_HEAP_ENABLE_TAIL_CHECK   0x10
#define FLG_HEAP_ENABLE_FREE_CHECK   0x20
#define FLG_HEAP_VALIDATE_PARAMETERS 0x40
#define NT_GLOBAL_FLAG_DEBUGGED (FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS)

void CheckNtGlobalFlag()
{
	PVOID pPeb = GetPEB();
	PVOID pPeb64 = GetPEB64();
	DWORD offsetNtGlobalFlag = 0;
#ifdef _WIN64
	offsetNtGlobalFlag = 0xBC;
#else
	offsetNtGlobalFlag = 0x68;
#endif
	DWORD NtGlobalFlag = *(PDWORD)((PBYTE)pPeb + offsetNtGlobalFlag);

	if (pPeb64)
	{
		DWORD NtGlobalFlagWow64 = *(PDWORD)((PBYTE)pPeb64 + 0xBC);

	}
}

PIMAGE_NT_HEADERS GetImageNtHeaders(PBYTE pImageBase)
{
	PIMAGE_DOS_HEADER pImageDosHeader = (PIMAGE_DOS_HEADER)pImageBase;
	return (PIMAGE_NT_HEADERS)(pImageBase + pImageDosHeader->e_lfanew);
}

PIMAGE_SECTION_HEADER FindRDataSection(PBYTE pImageBase)
{
	static const std::string rdata = ".rdata";
	PIMAGE_NT_HEADERS pImageNtHeaders = GetImageNtHeaders(pImageBase);
	PIMAGE_SECTION_HEADER pImageSectionHeader = IMAGE_FIRST_SECTION(pImageNtHeaders);
	int n = 0;
	for (; n < pImageNtHeaders->FileHeader.NumberOfSections; ++n)
	{
		if (rdata == (char*)pImageSectionHeader[n].Name)
		{
			break;
		}
	}
	return &pImageSectionHeader[n];
}

void CheckGlobalFlagsClearInProcess()
{
	PBYTE pImageBase = (PBYTE)GetModuleHandle(NULL);
	PIMAGE_NT_HEADERS pImageNtHeaders = GetImageNtHeaders(pImageBase);
	PIMAGE_LOAD_CONFIG_DIRECTORY pImageLoadConfigDirectory = (PIMAGE_LOAD_CONFIG_DIRECTORY)(pImageBase
		+ pImageNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress);

}

void CheckGlobalFlagsClearInFile()
{
	HANDLE hExecutable = INVALID_HANDLE_VALUE;
	HANDLE hExecutableMapping = NULL;
	PBYTE pMappedImageBase = NULL;
	__try
	{
		PBYTE pImageBase = (PBYTE)GetModuleHandle(NULL);
		PIMAGE_SECTION_HEADER pImageSectionHeader = FindRDataSection(pImageBase);
		TCHAR pszExecutablePath[MAX_PATH];
		DWORD dwPathLength = GetModuleFileName(NULL, pszExecutablePath, MAX_PATH);
		if (0 == dwPathLength) __leave;
		hExecutable = CreateFile(pszExecutablePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (INVALID_HANDLE_VALUE == hExecutable) __leave;
		hExecutableMapping = CreateFileMapping(hExecutable, NULL, PAGE_READONLY, 0, 0, NULL);
		if (NULL == hExecutableMapping) __leave;
		pMappedImageBase = (PBYTE)MapViewOfFile(hExecutableMapping, FILE_MAP_READ, 0, 0,
			pImageSectionHeader->PointerToRawData + pImageSectionHeader->SizeOfRawData);
		if (NULL == pMappedImageBase) __leave;
		PIMAGE_NT_HEADERS pImageNtHeaders = GetImageNtHeaders(pMappedImageBase);
		PIMAGE_LOAD_CONFIG_DIRECTORY pImageLoadConfigDirectory = (PIMAGE_LOAD_CONFIG_DIRECTORY)(pMappedImageBase
			+ (pImageSectionHeader->PointerToRawData
				+ (pImageNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress - pImageSectionHeader->VirtualAddress)));

	}
	__finally
	{
		if (NULL != pMappedImageBase)
			UnmapViewOfFile(pMappedImageBase);
		if (NULL != hExecutableMapping)
			CloseHandle(hExecutableMapping);
		if (INVALID_HANDLE_VALUE != hExecutable)
			CloseHandle(hExecutable);
	}
}
