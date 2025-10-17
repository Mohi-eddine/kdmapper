#pragma once

#include <Windows.h>

typedef struct _PE_INFO
{
	PBYTE ImageBase;
	SIZE_T ImageSize;
	BOOLEAN IsNtHeadersSkipped;
}PE_INFO, * PPE_INFO;

typedef struct _ENRTY_POINT_PARAMS
{
	ULONG64 Params[5];
}ENRTY_POINT_PARAMS, * PENRTY_POINT_PARAMS;

typedef struct _ASM_IO_INVOKE_SUB_CONTEXT
{
	UCHAR Irql; //set the desired IRQL
	PVOID InvokedFunction;
	PPE_INFO PeInfo;
	ULONG64 Params[5];
}ASM_IO_INVOKE_SUB_CONTEXT, * PASM_IO_INVOKE_SUB_CONTEXT;

#pragma pack(push,4)
typedef struct _ASM_IO_INVOKE_CONTEXT//size must be 0x2C
{
	PVOID KernelCaveFuncAddress;
	PASM_IO_INVOKE_SUB_CONTEXT SubContext;
	ULONG64 OriginalHookBytes;
	ULONG64 HookedFunctionAddress;
	PVOID ReturnBuffer;
	union {
		NTSTATUS Status;
		ULONG Magic;
	};
}ASM_IO_INVOKE_CONTEXT, * PASM_IO_INVOKE_CONTEXT;
#pragma pack(pop)

namespace kdmapper
{
	enum class AllocationMode
	{
		AllocatePool,
		AllocateIndependentPages
	};

	typedef bool (*mapCallback)(ULONG64* param1, ULONG64* param2, ULONG64 allocationPtr, ULONG64 allocationSize);

	//Note: if you set PassAllocationAddressAsFirstParam as true, param1 will be ignored
	ULONG64 MapDriver(HANDLE iqvw64e_device_handle, BYTE* data, PENRTY_POINT_PARAMS EntryParams, ULONG64 param2 = 0, bool free = false, bool destroyHeader = true, AllocationMode mode = AllocationMode::AllocatePool, mapCallback callback = nullptr, NTSTATUS* exitCode = nullptr);
}