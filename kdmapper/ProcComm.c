#include "ProcComm.h"

//RSA_KEY PublicKey = { 0 };

#ifdef PARENT_PROC
//parent proc
bool ParentProcCreateShareMem(PHANDLE phMapFile, PPROC_COMM_DATA* pSharedMemory)
{
    if (!phMapFile || !pSharedMemory)
        return false;

    *phMapFile = NULL;
    *pSharedMemory = NULL;

    // Step 1: Create shared memory
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // Use system paging file
        NULL,                    // Default security
        PAGE_READWRITE,          // Read/write access
        0,                       // Maximum object size (high-order DWORD)
        SHARED_MEMORY_SIZE,      // Maximum object size (low-order DWORD)
        SHARED_MEMORY_NAME       // Name of the shared memory
    );

    if (hMapFile == NULL) {
        //std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
        return false;
    }

    // Step 2: Map the shared memory into the current process
    PPROC_COMM_DATA _pSharedMemory = (PPROC_COMM_DATA)MapViewOfFile(
        hMapFile,                // Handle to the file mapping object
        SECTION_MAP_WRITE | SECTION_MAP_READ,     // Read/write access
        0,                       // File offset (high-order DWORD)
        0,                       // File offset (low-order DWORD)
        SHARED_MEMORY_SIZE       // Number of bytes to map
    );

    if (_pSharedMemory == NULL) {
        //std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return false;
    }
    *phMapFile = hMapFile;
    *pSharedMemory = _pSharedMemory;
    return true;
}

bool ParentProcCommStart(const TCHAR* ChildProc, TCHAR* Cmd, const TCHAR* ChildProcCurrentDir, PHANDLE hChildProc, PHANDLE hChildProcThread)
{
    if (!hChildProc || !hChildProcThread)
        return false;

    *hChildProc = NULL;
    *hChildProcThread = NULL;

    // Step 4: Start the child process
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    if (!CreateProcess(
        ChildProc,           // Path to the child process executable
        Cmd,                 // Command-line arguments
        NULL,                // Process security attributes
        NULL,                // Thread security attributes
        FALSE,               // Inherit handles
        0,                   // Creation flags
        NULL,                // Environment
        ChildProcCurrentDir, // Current directory
        &si,                 // Startup info
        &pi                  // Process information
    )) 
    {
        //std::cerr << "Failed to create child process: " << GetLastError() << std::endl;
       /* UnmapViewOfFile(pSharedMemory);
        //CloseHandle(hMapFile);*/
        return false;
    }

    *hChildProc = pi.hProcess;
    *hChildProcThread = pi.hThread;

    return true;
}

void ParentProcWaitForChildProc(HANDLE hChildProc)
{
    if (!hChildProc || hChildProc == INVALID_HANDLE_VALUE)
        return;
    //std::cout << "Parent process wrote to shared memory: " << (char*)pSharedMemory << std::endl;

    // Wait for the child process to finish
    WaitForSingleObject(hChildProc, INFINITE);
}

void ParentProcCommEnd(HANDLE hMapFile, HANDLE hChildProc, HANDLE hChildProcThread, PPROC_COMM_DATA pSharedMemory)
{ // Clean up
    if(pSharedMemory)  
        UnmapViewOfFile(pSharedMemory);

    if(hMapFile && hMapFile != INVALID_HANDLE_VALUE)
        CloseHandle(hMapFile);

    if (hChildProc && hChildProc != INVALID_HANDLE_VALUE)
        CloseHandle(hChildProc);

    if (hChildProcThread && hChildProcThread != INVALID_HANDLE_VALUE)
        CloseHandle(hChildProcThread);
}
#endif

#ifdef CHILD_PROC
////////////////////////////////////////////////////////////////////////////////////////////////
//child proc

bool ChildProcCommStart(PHANDLE pMapFileHandle, PPROC_COMM_DATA* pSharedMemory)
{
    if (!pSharedMemory || !pMapFileHandle)
        return false;

    *pSharedMemory = NULL;
    *pMapFileHandle = NULL;

    // Step 1: Open the shared memory created by the parent process
    HANDLE hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,     // Read/write access
        FALSE,                   // Do not inherit the handle
        SHARED_MEMORY_NAME       // Name of the shared memory
    );

    if (hMapFile == NULL) {
        //std::cerr << "Could not open file mapping object: " << GetLastError() << std::endl;
        return false;
    }

    // Step 2: Map the shared memory into the current process
    PPROC_COMM_DATA _pSharedMemory = (PPROC_COMM_DATA)MapViewOfFile(
        hMapFile,                // Handle to the file mapping object
        FILE_MAP_ALL_ACCESS,     // Read/write access
        0,                       // File offset (high-order DWORD)
        0,                       // File offset (low-order DWORD)
        SHARED_MEMORY_SIZE       // Number of bytes to map
    );

    if (_pSharedMemory == NULL) {
        //std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return false;
    }

    *pSharedMemory = _pSharedMemory;
    *pMapFileHandle = hMapFile;
    return true;
}

    // Step 3: Read data from shared memory
    //std::cout << "Child process read from shared memory: " << (char*)pSharedMemory << std::endl;

    //// Step 4: Write a response to shared memory
    //const char* response = "Hello from Child Process!";
    //memcpy(pSharedMemory, response, strlen(response) + 1);

void ChildProcCommEnd(HANDLE MapFileHandle ,PPROC_COMM_DATA pSharedMemory)
{
// Clean up
    if(pSharedMemory)
        UnmapViewOfFile(pSharedMemory);

    if(MapFileHandle && MapFileHandle != INVALID_HANDLE_VALUE)
        CloseHandle(MapFileHandle);
}

#endif