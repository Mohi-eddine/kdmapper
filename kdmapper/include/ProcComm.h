#pragma once

#include <windows.h>
#include "include/Prime/RSA.h"

#define SHARED_MEMORY_NAME L"Global\\CommMem"

typedef struct _PROC_COMM_DATA
{
    RSA_KEY PublicKey;
    gint Data;
   /* int ExitCode;
    BOOLEAN IsChildDone;*/
}PROC_COMM_DATA , *PPROC_COMM_DATA;

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