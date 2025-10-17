#pragma once

#include <windows.h>
#include "Prime/RSA.h"

/*User Includes*/

#define SHARED_MEMORY_NAME L"Global\\CommMem"

/*User Structs*/
///*TheGapOfRohan Mapper*/
typedef struct _PE_ENTRY_PARAMS
{
    ULONG64 Params[5];
}PE_ENTRY_PARAMS, * PPE_ENTRY_PARAMS;

/*this struct can be modified as needed*/
typedef struct _PROC_COMM_DATA
{
    RSA_KEY PublicKey;
    gint FileDecryptionKey;
    PE_ENTRY_PARAMS EntryPointParams;//don't attempt to access pointers on this array from a child proc
    WCHAR FileExtension[5];
    BOOLEAN IsEncrypted;//this is related to the RSA_KEY and the FileDecryptionKey
    BOOLEAN SkipNtHeaders;
    /* you can specify more here*/
}PROC_COMM_DATA, * PPROC_COMM_DATA;

#define SHARED_MEMORY_SIZE (sizeof(PROC_COMM_DATA))
//extern RSA_KEY PublicKey;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PARENT_PROC
    //parent proc
    bool ParentProcCreateShareMem(PHANDLE phMapFile, PPROC_COMM_DATA* pSharedMemory);
    bool ParentProcCommStart(const TCHAR* ChildProc, TCHAR* Cmd, const TCHAR* ChildProcCurrentDir, PHANDLE hChildProc, PHANDLE hChildProcThread);
    void ParentProcWaitForChildProc(HANDLE hChildProc);
    void ParentProcCommEnd(HANDLE hMapFile, HANDLE hChildProc, HANDLE hChildProcThread, PPROC_COMM_DATA pSharedMemory);
#endif

#ifdef CHILD_PROC
    //child proc
    bool ChildProcCommStart(PHANDLE pMapFileHandle, PPROC_COMM_DATA* pSharedMemory);
    void ChildProcCommEnd(HANDLE MapFileHandle, PPROC_COMM_DATA pSharedMemory);
#endif

#ifdef __cplusplus
}
#endif