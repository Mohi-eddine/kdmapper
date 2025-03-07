/*
#####################
# MoHieDDiNNe Codes #
#####################
*/

#pragma once
///////////////////////////////////////////////////////////////////////////////
//
// some of the following functions are imported made by: 
// 
// Author: Oleg Starodumov
// https://www.debuginfo.com/examples/dbghelpexamples.html
//

#include "DataStructs.h"
#include <dbghelp.h>
#pragma comment( lib, "dbghelp.lib" )


#ifdef UNICODE
#define NTOSKRNL_PATH		L"C:\\Windows\\System32\\ntoskrnl.exe"//L".\\Tools\\Symbols\\ntkrnlmp.pdb"
#define WDFILTER_PATH		L"C:\\Windows\\System32\\drivers\\wd\\WdFilter.sys"//L".\\Tools\\Symbols\\WdFilter.pdb"
#define CIDLL_PATH			L"C:\\Windows\\System32\\ci.dll"//L".\\Tools\\Symbols\\ci.pdb"

#define SYM_OFFSETS_PATH		L"SymbolsOffset.txt"

#define PATH_TYPE			PCWSTR
#else
#define NTOSKRNL_PATH		"C:\\Windows\\System32\\ntoskrnl.exe"//L".\\Tools\\Symbols\\ntkrnlmp.pdb"
#define WDFILTER_PATH		"C:\\Windows\\System32\\drivers\\wd\\WdFilter.sys"//L".\\Tools\\Symbols\\WdFilter.pdb"
#define CIDLL_PATH			"C:\\Windows\\System32\\ci.dll"//L".\\Tools\\Symbols\\ci.pdb"


#define FUNCOFFSET_PATH		"SymbolsOffset.txt"

#define PATH_TYPE			PCSTR
#endif


inline char* StrConcat(const char* s1, const char* s2)
{
    if (!s1 || !s2)
        return NULL;

    const size_t Len1 = strlen(s1);
    const size_t Len2 = strlen(s2);
    if (!Len1 || !Len2)
        return NULL;

    char* New = (char*)malloc(Len1 + Len2 + 1);
    if (!New)
        return NULL;

    strcpy_s(New, Len1, s1);
    strcat_s(New, Len1 + Len2, s2);
    return New;
}

inline wchar_t* WStrConcat(const wchar_t* s1, const wchar_t* s2)
{
    if (!s1 || !s2)
        return NULL;

    const size_t Len1 = wcslen(s1);
    const size_t Len2 = wcslen(s2);
    if (!Len1 || !Len2)
        return NULL;

    wchar_t* New = (wchar_t*)malloc(((Len1 + Len2) * sizeof(wchar_t)) + sizeof(wchar_t));
    if (!New)
        return NULL;

    wcscpy_s(New, Len1 + 1, s1);
    wcscat_s(New, Len1 + Len2 + 1, s2);
    return New;
}

BOOL SetSymbolsPath();

BOOL InitSymServer(
   IN PCWCHAR StoragePath
);

BOOL GetPdbFile(
    IN PCWCHAR TargetBinPath,
    OUT PWSTR OutPdbFilePath
);

BOOLEAN GenerateOffsetFile(
	void
);

BOOLEAN InitKernelSymbolsList(
	IN PATH_TYPE FilePath,
	IN OUT PSYM_INFO_ARRAY pSymbolsArray
);

BOOLEAN GetFileParams(IN 
	PATH_TYPE pFileName,
	OUT uintptr_t* BaseAddr,
	OUT DWORD* FileSize
);

BOOLEAN _GetFileSize(
	IN PATH_TYPE pFileName,
	OUT DWORD* FileSize
);

#ifndef NDEBUG
void ShowSymbolInfo(
	IN uintptr_t ModBase
);
#endif