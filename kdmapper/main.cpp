#ifndef KDLIBMODE

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <TlHelp32.h>

#include "kdmapper.hpp"
#include "utils.hpp"
#include "intel_driver.hpp"

#include "SimpleEncryption.h"
#include "ProcComm.h"

HANDLE iqvw64e_device_handle;

#define FILE_INFO_LENGTH (sizeof(__m128i)*2)

LONG WINAPI SimplestCrashHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	if (ExceptionInfo && ExceptionInfo->ExceptionRecord)
		Log(L"[!!] Crash at addr 0x" << ExceptionInfo->ExceptionRecord->ExceptionAddress << L" by 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::endl);
	else
		Log(L"[!!] Crash" << std::endl);

	if (iqvw64e_device_handle)
		intel_driver::Unload(iqvw64e_device_handle);

	return EXCEPTION_EXECUTE_HANDLER;
}

int paramExists(const int argc, wchar_t** argv, const wchar_t* param) {
	size_t plen = wcslen(param);
	for (int i = 0; i < argc; i++) {
		if (wcslen(argv[i]) == plen + 1ull && _wcsicmp(&argv[i][1], param) == 0 && argv[i][0] == '/') { // with slash
			return i;
		}
		else if (wcslen(argv[i]) == plen + 2ull && _wcsicmp(&argv[i][2], param) == 0 && argv[i][0] == '-' && argv[i][1] == '-') { // with double dash
			return i;
		}
	}
	return -1;
}

bool callbackExample(ULONG64* param1, ULONG64* param2, ULONG64 allocationPtr, ULONG64 allocationSize) {
	UNREFERENCED_PARAMETER(param1);
	UNREFERENCED_PARAMETER(param2);
	UNREFERENCED_PARAMETER(allocationPtr);
	UNREFERENCED_PARAMETER(allocationSize);
	Log("[+] Callback example called" << std::endl);
	
	/*
	This callback occurs before call driver entry and
	can be useful to pass more customized params in 
	the last step of the mapping procedure since you 
	know now the mapping address and other things
	*/
	return true;
}

DWORD getParentProcess()
{
	HANDLE hSnapshot;
	PROCESSENTRY32 pe32;
	DWORD ppid = 0, pid = GetCurrentProcessId();

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	__try {
		if (hSnapshot == INVALID_HANDLE_VALUE) __leave;

		ZeroMemory(&pe32, sizeof(pe32));
		pe32.dwSize = sizeof(pe32);
		if (!Process32First(hSnapshot, &pe32)) __leave;

		do {
			if (pe32.th32ProcessID == pid) {
				ppid = pe32.th32ParentProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe32));

	}
	__finally {
		if (hSnapshot && hSnapshot != INVALID_HANDLE_VALUE) CloseHandle(hSnapshot);
	}
	return ppid;
}

//Help people that don't understand how to open a console
void PauseIfParentIsExplorer() {
#ifndef DEBUG
//	DWORD explorerPid = 0;
//	GetWindowThreadProcessId(GetShellWindow(), &explorerPid);
//	DWORD parentPid = getParentProcess();
//	if (parentPid == explorerPid) {
//		Log(L"[+] Pausing to allow for debugging" << std::endl);
//		Log(L"[+] Press enter to close" << std::endl);
//		std::cin.get();
//	}
//#else
//	Log(L"[+] Pausing to allow for debugging" << std::endl);
//	Log(L"[+] Press enter to close" << std::endl);
//	std::cin.get();
#endif
}

void help() {
	Log(L"\r\n\r\n[!] Incorrect Usage!" << std::endl);
#ifdef PDB_OFFSETS
	Log(L"[+] Usage: kdmapper.exe [--dec --pwd \"password\"][--Name][--DontUpdateOffset | --OffsetsPath \"FilePath\"][--free | --indPages][--PassAllocationPtr] driver" << std::endl); 
#else
	Log(L"[+] Usage: kdmapper.exe [--dec --pwd \"password\"][--Name][--free | --indPages][--PassAllocationPtr] driver" << std::endl);
#endif
	PauseIfParentIsExplorer();
}

int GetEncryptionMode(const int argc, wchar_t** argv)
{
	int EncParamIdx = paramExists(argc, argv, L"enc");
	int DecParamIdx = paramExists(argc, argv, L"dec");


	if (EncParamIdx >= 0 && DecParamIdx >= 0)
	{
		Print("[-] Error: Both enc and dec got provided at the same time\n");
		return -1;
	}
	else if (EncParamIdx < 0 && DecParamIdx < 0)
	{
		Print("[-] Error: Neither enc nor dec got provided\n");
		return -1;
	}
	int ParamIdx = -1;
	int Mode = -1;
	if (EncParamIdx >= 0)
	{
		ParamIdx = EncParamIdx;
		Mode = 1;
	}
	else
	{
		ParamIdx = DecParamIdx;
		Mode = 2;
	}

	/*wchar_t KeyMode = *argv[ParamIdx + 1];
	Print("\n====================================================\n");
	if (KeyMode == L'1')
	{
		Print(">> 16 Char Key Mode\n");
		*KeySize = 128 / 8;
	}
	else if (KeyMode == L'2')
	{
		Print(">> 24 Char Key Mode\n");
		*KeySize = 192 / 8;
	}
	else if (KeyMode == L'3')
	{
		Print(">> 32 Char Key Mode\n");
		*KeySize = 256 / 8;
	}
	else
	{
		Print("Warning: Defaulting to 16 Char Key Mode\n");
		*KeySize = 128 / 8;
	}
	Print("====================================================\n\n");*/

	return Mode;
}

bool GetEncryptionKey(const int argc, wchar_t** argv, __m128i Key[2])
{
	if (!Key)
	{
		Print("[-] Error: Key is NULL\n");
		return FALSE;
	}
	Key[0] = _mm_xor_si128(Key[0], Key[0]);
	Key[1] = _mm_xor_si128(Key[1], Key[1]);

	int PwdParamIdx = paramExists(argc, argv, L"pwd");

	if (PwdParamIdx < 0)
	{
		Print("[-] Error: No Password Is Provided.\n");
		return FALSE;
	}

	bool Result = FALSE;
	DWORD KeyHash = 0;
	do
	{
		if (PwdParamIdx >= 0)
		{
			//change this later to prompt for user input instead
			//Print("Password: %ls\n", argv[PwdParamIdx + 1]);
			Print("[+] Generating key Based On Password...\n");
			size_t PasswordSize = wcslen(argv[PwdParamIdx + 1]);
			if (PasswordSize < 8)
			{
				Print("[-] Error: Password too short.\n");
				break;
			}
			KeyHash = KeyGenerator(argv[PwdParamIdx + 1], PasswordSize, Key);
			Print("\n====================================================\n");
			Print("[+] -> Key: 256Bit.\n[+] -> KeyHash(Crc32): 0x%X\n", KeyHash);
			Print("====================================================\n\n");
			Result = KeyHash != 0;
		}
		else
		{
			Print("[-] Error: No password was provided.\n");
			////generate random key
			//Print("Generating random key...\n");
			//KeyHash = RandKeyGenerator(Key);
			//Print("\n====================================================\n");
			//Print("-> Key: %ls");
			//for (size_t i = 0; i < KeySize; i++)
			//{
			//	Print("%02X ", ((BYTE*)Key)[i]);
			//}
			//Print("\n-> KeyHash: 0x%X\n", KeyHash);
			//Print("====================================================\n\n");
			//Result = KeyHash != 0;
		}

		if (!Result)
		{
			Print("[-] Error: Failed to Obtain Cipher key.\n");
			break;
		}
		Print("[+] Successfully Obtained The Cipher Key\n");
	} while (FALSE);
	return Result;
}

//generated keys 
static BYTE Mod_Aob[] = { 0x17, 0xF0, 0x03, 0x7F, 0xB5, 0xA3, 0x6A, 0xEE, 0x96, 0x4D, 0x24, 0x88, 0x18, 0xD3, 0x29, 0x6C, 0x32, 0xA6, 0x78, 0x8F, 0x57, 0xF8, 0xB0, 0x2E, 0x7A, 0xAE, 0x35, 0xB6, 0x81, 0x22, 0xAC, 0xC6, 0x67, 0xD6, 0x0D, 0x91, 0x6E, 0xBA, 0xC2, 0xBE, 0x3C, 0x00, 0x67, 0xEA, 0x1C, 0xC7, 0x6D, 0x9E, 0xC3, 0x30, 0x73, 0x98, 0x4C, 0x11, 0x0C, 0x67, 0x49, 0xDC, 0xF6, 0x60, 0x4E, 0xBC, 0xC2, 0xA7, 0xC1, 0x10, 0xD3, 0x44, 0x6F, 0xCC, 0x8B, 0xA6, 0x92, 0x62, 0xD9, 0x6D, 0xBB, 0x9B, 0x1A, 0xA0, 0x45, 0xFF, 0x7E, 0x1E, 0x17, 0x8F, 0x3F, 0x5A, 0xE1, 0x30, 0x1D, 0x0C, 0x04, 0xCD, 0xB6, 0x21, 0xC4, 0x71, 0x4F, 0x12, 0x98, 0x49, 0x76, 0xF7, 0xDE, 0x6B, 0x09, 0xB7, 0x85, 0x11, 0x2D, 0x6D, 0x32, 0xEC, 0x80, 0x03, 0x1F, 0xBF, 0x45, 0x1E, 0xF3, 0x50, 0x27, 0x67, 0x59, 0x27, 0xF1, 0x6A, 0x43, 0xAD, 0x37, 0x83, 0x4A, 0xD6, 0xDB, 0x83, 0x6F, 0xD1, 0xB3, 0x3F, 0xEF, 0xB3, 0x3E, 0x49, 0xF4, 0x92, 0x30, 0x40, 0x2D, 0x47, 0x80, 0x3B, 0x60, 0xDA, 0xFA, 0x56, 0xD4, 0x85, 0x63, 0xFF, 0x10, 0x92, 0x16, 0x85, 0xFF, 0xA1, 0xF1, 0xB7, 0x1B, 0x0C, 0xD4, 0x89, 0x54, 0x33, 0xF2, 0x32, 0x75, 0xA1, 0xF1, 0x17, 0xC5, 0xC9, 0x91, 0x39, 0x5E, 0x48, 0xE5, 0x2F, 0x49, 0x11, 0x1D, 0xD9, 0xEE, 0x32, 0x8A, 0x5D, 0x73, 0xCB, 0xB0, 0xAF, 0x8B, 0xDD, 0x14, 0xE0, 0xCB, 0x4D, 0x50, 0x2A, 0x62, 0xEA, 0xA8, 0x75, 0xBD, 0x85, 0x60, 0x43, 0xDD, 0x1F, 0x60, 0x94, 0x7C, 0xA9, 0x1B, 0xED, 0x12, 0xC1, 0xB0, 0x52, 0xAE, 0x73, 0xE4, 0x8E, 0x4B, 0x6B, 0x17, 0x1B, 0x36, 0x26, 0xE7, 0xDD, 0x04, 0xE6, 0x55, 0x4E, 0xE4, 0xF4, 0x93, 0x1A, 0x16, 0x6A, 0xE8, 0x59, 0x7A, 0xFF, 0x06, 0x55, };
static BYTE DecKey_Aob[] = { 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, };

int wmain(const int argc, wchar_t** argv) {
	SetUnhandledExceptionFilter(SimplestCrashHandler);

#ifdef PDB_OFFSETS
	bool UpdateOffset = !(paramExists(argc, argv, L"DontUpdateOffset") >= 0);
	int FilePathParamIdx = paramExists(argc, argv, L"OffsetsPath");
	if (FilePathParamIdx >= 0)
	{
		SymbolsOffsetFilePath = argv[FilePathParamIdx + 1];
#ifdef UNICODE
		Print("[+] Setting Offsets File Path To: %ls\n", SymbolsOffsetFilePath);
#else
		Print("[+] Setting Offsets File Path To: %s\n", SymbolsOffsetFilePath);
#endif
	}

	CSymInfo SymInfo(&SymbolsInfoArray, UpdateOffset);
	if (!SymInfo.m_IsValid) {
		Log(L"[-] Error: Failed To Get Symbols Info.\n");
		PauseIfParentIsExplorer();
		return -1;
	}
#endif

	bool IsEncrypted = false;
	bool IsChildProc = false;
	
	PPROC_COMM_DATA ProcData = NULL;
	HANDLE hMapFile = NULL;
	__m128i Key[2];
	if (ChildProcCommStart(&hMapFile, &ProcData))
	{
		Print("[+] Started From Parent Proc\n");
		RSA_KEY PublicKey = { 0 };
		if(ggint_is_zero(ProcData->PublicKey.mod) && ggint_is_zero(ProcData->PublicKey.exp))
		{
			memcpy(&PublicKey.mod, Mod_Aob, sizeof(Mod_Aob));
			memcpy(&PublicKey.exp, DecKey_Aob, sizeof(DecKey_Aob));
		}
		else
		{
			memcpy(&PublicKey.mod, &ProcData->PublicKey.mod, sizeof(gint));
			memcpy(&PublicKey.exp, &ProcData->PublicKey.exp, sizeof(gint));
		}

		gint DecryptedData;
		rsa_decrypt_single(&PublicKey, ProcData->Data, DecryptedData);
		ChildProcCommEnd(hMapFile, ProcData);
		if (ggint_is_zero(DecryptedData))
		{
			Print("[-] Error: Failed to get decryption key.\n");
			return -1;
		}

		IsChildProc = true;

		Print("[+] Generating key Based On Password...\n");
		size_t PasswordSize = wcslen((wchar_t*)DecryptedData);
		if (PasswordSize < 8)
		{
			Print("[-] Error: Password too short.\n");
			return -1;
		}
		DWORD KeyHash = KeyGenerator((wchar_t*)DecryptedData, PasswordSize, Key);
		ggint_zero(DecryptedData);
		Print("\n====================================================\n");
		Print("[+] -> Key: 256Bit.\n[+] -> KeyHash(Crc32): 0x%X\n", KeyHash);
		Print("====================================================\n\n");
		if (!KeyHash)
		{
			Print("[-] Failed To Generate Key\n");
			return -1;
		}
	}
	else
	{
		Print("[+] Normal Exec\n");

	}
	
	int Mode = GetEncryptionMode(argc, argv);
	if (Mode > 0)
	{
		if (!IsSSE42Supported())
		{
			printf("[-] SSE4.2 Instruction Sets Are Not Supported By Your System.\n");
			return -1;
		}

		if (!IsSha256Supported())
		{
			printf("[!] Warning: SHA-256 hardware acceleration is NOT supported.\n-> Using Software SHA-256.\n");
		}
		if(!IsChildProc)
		{
			if (!GetEncryptionKey(argc, argv, Key))
			{
				Print("[-] Error: Failed to get decryption key.\n");
				return -1;
			}
		}
		IsEncrypted = true;
	}

	int DriverNameIdx = paramExists(argc, argv, L"Name");
	wchar_t* DriverName = NULL;
	if (DriverNameIdx >= 0)
	{
		DriverName = argv[DriverNameIdx + 1];
		Log("[+] Driver Name Is Set To: " << DriverName << "\n");
	}

	bool free = paramExists(argc, argv, L"free") >= 0;
	bool indPagesMode = paramExists(argc, argv, L"indPages") >= 0;
	bool passAllocationPtr = paramExists(argc, argv, L"PassAllocationPtr") >= 0;

	if (free) {
		Log(L"[+] Free pool memory after usage enabled" << std::endl);
	}

	if (indPagesMode) {
		Log(L"[+] Allocate Independent Pages mode enabled" << std::endl);
	}

	if (free && indPagesMode) {
		Log(L"[-] Can't use --free and --indPages at the same time" << std::endl);
		help();
		return -1;
	}

	if (passAllocationPtr) {
		Log(L"[+] Pass Allocation Ptr as first param enabled" << std::endl);
	}

	int drvIndex = -1;
	for (int i = 0; i < argc; i++) {
		if (std::filesystem::path(argv[i]).extension().string().compare(".sys") == 0) {
			drvIndex = i;
			break;
		}
	}

	if (drvIndex < 0) {
		help();
		return -1;
	}

	const std::wstring driver_path = argv[drvIndex];

	if (!std::filesystem::exists(driver_path)) {
		Log(L"[-] File " << driver_path << L" doesn't exist" << std::endl);
		PauseIfParentIsExplorer();
		return -1;
	}
	std::vector<uint8_t> raw_image = { 0 };
	if (!utils::ReadFileToMemory(driver_path, &raw_image)) {
		Log(L"[-] Failed to read image to memory" << std::endl);
		PauseIfParentIsExplorer();
		return -1;
	}
	if (IsEncrypted)
	{
		__m128i FileSha256Hash[2];
		SHA256_SIMD_CTX Ctx;
		InitSha256(&Ctx);
		size_t AlignedFileSize = raw_image.size() - FILE_INFO_LENGTH;//exclude the length of file info from file size so it wont be added twice later

		Print("[+] Decrypting...\n");
		if (!DecryptBytes(raw_image.data(), raw_image.size(), Key))
		{
			memset(Key, 0, sizeof(Key));
			Print("[-] Error: Failed to decrypt file.\n");
			PauseIfParentIsExplorer();
			return -1;
		}
		memset(Key, 0, sizeof(Key));
		BYTE* Buffer = raw_image.data();

		//compute the sha256 after decryption
		ComputeSha256(&Ctx, (const BYTE*)Buffer, AlignedFileSize, FileSha256Hash);
		FileSha256Hash[0] = _mm_xor_si128(FileSha256Hash[0], FileSha256Hash[1]);

		__m128i CmpResult = _mm_cmpeq_epi64(*(__m128i*)(Buffer + AlignedFileSize), FileSha256Hash[0]);
		if ((CmpResult.m128i_u64[0] != -1) || (CmpResult.m128i_u64[1] != -1))
		{
			Print("[-] Error: Decryption failed due to file integrity check failure.\n");
			PauseIfParentIsExplorer();
			return 2;
		}

		size_t FileSize = *(size_t*)(Buffer + (AlignedFileSize + sizeof(FileSha256Hash[0])));
		if (FileSize > AlignedFileSize)
		{
			Print("[-] Error: File size is larger than expected.\n");
			PauseIfParentIsExplorer();
			return 3;
		}
		
		raw_image.resize(FileSize);
		Print("[+] Successfully Decrypted The File\n");
	}

	iqvw64e_device_handle = intel_driver::Load();

	if (iqvw64e_device_handle == INVALID_HANDLE_VALUE) {
		PauseIfParentIsExplorer();
		return -1;
	}

	kdmapper::AllocationMode mode = kdmapper::AllocationMode::AllocatePool;

	if (indPagesMode) {
		mode = kdmapper::AllocationMode::AllocateIndependentPages;
	}

	NTSTATUS exitCode = 0;
	bool IsMapped = kdmapper::MapDriver(iqvw64e_device_handle, raw_image.data(), 0, (ULONG64)DriverName, free, true, mode, passAllocationPtr, callbackExample, &exitCode);
	if(IsEncrypted)
	{
		srand((unsigned int)time(NULL));
		memset(raw_image.data(), rand(), raw_image.size());
		raw_image.erase(raw_image.begin(), raw_image.end());
	}
	if (!IsMapped)
	{
		Log(L"[-] Failed to map " << driver_path << std::endl);
		intel_driver::Unload(iqvw64e_device_handle);
		PauseIfParentIsExplorer();
		return -1;
	}
	
	if (!NT_SUCCESS(exitCode) && (0xC0000035 != exitCode))//STATUS_OBJECT_NAME_COLLISION 0xC0000035 
	{
		Log(L"[-] Driver returned error code: 0x" << std::hex << exitCode << std::endl);
		intel_driver::Unload(iqvw64e_device_handle);
		PauseIfParentIsExplorer();
		return -1;
	}

	if (!intel_driver::Unload(iqvw64e_device_handle)) {
		Log(L"[-] Warning failed to fully unload vulnerable driver " << std::endl);
		PauseIfParentIsExplorer();
	}
	Log(L"[+] success" << std::endl);

#ifdef DEBUG
	PauseIfParentIsExplorer();
#endif

	return 0;
}

#endif