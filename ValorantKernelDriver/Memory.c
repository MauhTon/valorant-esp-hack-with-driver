#include <math.h>
#include "Memory.h"
#include "Dependencies.h"
#include "Helpers.h"
#include <ntstatus.h>
#include <minwindef.h>

#define LOWORD(l) ((WORD)(l))
#define FLT_MAX  3.402823466e+38F;
#define WORDn(x, n) (*((unsigned short *)&(x) + n))
#define LOWORD(x) WORDn(x, 0)

// Globals
static LARGE_INTEGER Timeout;

// Shared Memory Vars
const WCHAR g_SharedSectionName[] = L"\\BaseNamedObjects\\OverflowRW";
SECURITY_DESCRIPTOR SecDescriptor;
PVOID	Buffer = NULL;
ULONG	DaclLength;
PACL	Dacl;
HANDLE	g_Section = NULL;
PVOID   g_SharedSection = NULL;

// Vars
LARGE_INTEGER Timeout;
DWORD64		KernelBase = NULL;
DWORD64		BaseAddrR6 = NULL;
PEPROCESS	R6Process = NULL;
ULONG		PID_R6 = 0;

NTSTATUS CreateShareMemory()
{
	DbgPrint("Creating Memory.\n");
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	Status = RtlCreateSecurityDescriptor(&SecDescriptor, SECURITY_DESCRIPTOR_REVISION);
	if (!NT_SUCCESS(Status))
	{
#ifdef _DEBUG
		DbgPrintEx(0, 0, "RtlCreateSecurityDescriptor failed: %p\n", Status);
#endif
		return Status;
	}

	DaclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) * 3 + RtlLengthSid(SeExports->SeLocalSystemSid) + RtlLengthSid(SeExports->SeAliasAdminsSid) +
		RtlLengthSid(SeExports->SeWorldSid);

	Dacl = ExAllocatePoolWithTag(PagedPool, DaclLength, 'lcaD');

	if (Dacl == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
#ifdef _DEBUG
		DbgPrintEx(0, 0, "ExAllocatePoolWithTag failed: %p\n", Status);
#endif
	}

	Status = RtlCreateAcl(Dacl, DaclLength, ACL_REVISION);

	if (!NT_SUCCESS(Status))
	{
		ExFreePool(Dacl);
#ifdef _DEBUG
		DbgPrintEx(0, 0, "RtlCreateAcl failed: %p\n", Status);
#endif
		return Status;
	}

	Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeExports->SeWorldSid);

	if (!NT_SUCCESS(Status))
	{

		ExFreePool(Dacl);
		DbgPrintEx(0, 0, "RtlAddAccessAllowedAce SeWorldSid failed: %p\n", Status);
		return Status;
	}

	Status = RtlAddAccessAllowedAce(Dacl,
		ACL_REVISION,
		FILE_ALL_ACCESS,
		SeExports->SeAliasAdminsSid);

	if (!NT_SUCCESS(Status))
	{
		ExFreePool(Dacl);
		DbgPrintEx(0, 0, "RtlAddAccessAllowedAce SeAliasAdminsSid failed  : %p\n", Status);
		return Status;
	}

	Status = RtlAddAccessAllowedAce(Dacl,
		ACL_REVISION,
		FILE_ALL_ACCESS,
		SeExports->SeLocalSystemSid);

	if (!NT_SUCCESS(Status))
	{
		ExFreePool(Dacl);
		DbgPrintEx(0, 0, "RtlAddAccessAllowedAce SeLocalSystemSid failed  : %p\n", Status);
		return Status;
	}

	Status = RtlSetDaclSecurityDescriptor(&SecDescriptor,
		TRUE,
		Dacl,
		FALSE);

	if (!NT_SUCCESS(Status))
	{
		ExFreePool(Dacl);
		DbgPrintEx(0, 0, "RtlSetDaclSecurityDescriptor failed  : %p\n", Status);
		return Status;
	}

	UNICODE_STRING SectionName = { 0 };
	RtlInitUnicodeString(&SectionName, g_SharedSectionName);

	OBJECT_ATTRIBUTES ObjAttributes = { 0 };
	InitializeObjectAttributes(&ObjAttributes, &SectionName, OBJ_CASE_INSENSITIVE, NULL, &SecDescriptor);

	LARGE_INTEGER lMaxSize = { 0 };
	lMaxSize.HighPart = 0;
	lMaxSize.LowPart = 1044 * 10;

	/* Begin Mapping */
	Status = ZwCreateSection(&g_Section, SECTION_ALL_ACCESS, &ObjAttributes, &lMaxSize, PAGE_READWRITE, SEC_COMMIT, NULL);
	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Create Section Failed. Status: %p\n", Status);
		return Status;
	}

	//-----------------------------------------------------------------------------	
	//	 ZwMapViewOfSection
	//	-lMaxSize is the ammount of 'Room' the MapViewOfSection will look at
	//	-ViewSize is how much of the 'Room' will be mapped (if 0 then starts at beggining)
	//-----------------------------------------------------------------------------	

	SIZE_T ulViewSize = 0;
	Status = ZwMapViewOfSection(g_Section, NtCurrentProcess(), &g_SharedSection, 0, lMaxSize.LowPart, NULL, &ulViewSize, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);
	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Map View Section Failed. Status: %p\n", Status);
		ZwClose(g_Section); //Close Handle
		return Status;
	}

	DbgPrintEx(0, 0, "Shared Memory Created.\n\n");
	ExFreePool(Dacl);

	return Status;
}


NTSTATUS ReadSharedMemory()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (!g_Section)
		return;

	if (g_SharedSection)
		ZwUnmapViewOfSection(NtCurrentProcess(), g_SharedSection);

	SIZE_T ulViewSize = 1044 * 10;
	Status = ZwMapViewOfSection(g_Section, NtCurrentProcess(), &g_SharedSection, 0, ulViewSize, NULL, &ulViewSize, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);
	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Read Shared Memory Failed. %p\n", Status);
		ZwClose(g_Section); //Close Handle
		return;
	}

	return Status;
}

VOID CloseMemory()
{
	// Free Section Memory
	if (g_SharedSection)
		ZwUnmapViewOfSection(NtCurrentProcess(), g_SharedSection);

	// Closing Handle
	if (g_Section)
		ZwClose(g_Section);
}

NTSTATUS Write(ULONG pid, DWORD64 SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
	SIZE_T Bytes = 0;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrintEx(0, 0, "\nWrite Address:%p\n", SourceAddress);
	DbgPrintEx(0, 0, "Write Value  :%p\n", TargetAddress);
	DbgPrintEx(0, 0, "Write szAddress:%x\n", Size);

	PEPROCESS process;
	PsLookupProcessByProcessId((HANDLE)pid, &process);

	KAPC_STATE state;
	KeStackAttachProcess(process, &state);
	Status = MmCopyVirtualMemory(PsGetCurrentProcess(), TargetAddress, R6Process, SourceAddress, Size, KernelMode, &Bytes);
	KeUnstackDetachProcess(&state);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Bytes Copied:%x\n", Bytes);
		DbgPrintEx(0, 0, "Write Failed:%p\n", Status);
		return Status;
	}

	return Status;
}

NTSTATUS WriteByte(DWORD64 SourceAddress, unsigned char TargetAddress, SIZE_T Size)
{
	SIZE_T Bytes;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrintEx(0, 0, "\nWrite Address:%p\n", SourceAddress);
	DbgPrintEx(0, 0, "Write szAddress:%x\n", Size);

	Status = MmCopyVirtualMemory(PsGetCurrentProcess(), &TargetAddress, R6Process, SourceAddress, Size, KernelMode, &Bytes);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Bytes Copied:%x\n", Bytes);
		DbgPrintEx(0, 0, "Write Failed:%p\n", Status);
		return Status;
	}

	return Status;
}

NTSTATUS WriteFloat(DWORD64 SourceAddress, float* TargetAddress, SIZE_T Size)
{
	SIZE_T Bytes;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrintEx(0, 0, "\nWrite Address:%p\n", SourceAddress);
	DbgPrintEx(0, 0, "Write szAddress:%x\n", Size);

	Status = MmCopyVirtualMemory(PsGetCurrentProcess(), TargetAddress, R6Process, SourceAddress, Size, KernelMode, &Bytes);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Write Failed:%p\n", Status);
		return Status;
	}

	return Status;
}

NTSTATUS WriteInt(DWORD64 SourceAddress, int TargetAddress, SIZE_T Size)
{
	SIZE_T Bytes;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrintEx(0, 0, "\nWrite Address:%p\n", SourceAddress);
	DbgPrintEx(0, 0, "Write szAddress:%x\n", Size);

	Status = MmCopyVirtualMemory(PsGetCurrentProcess(), &TargetAddress, R6Process, SourceAddress, Size, KernelMode, &Bytes);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Write Failed:%p\n", Status);
		return Status;
	}

	return Status;
}

void Read(DWORD64 SourceAddress, PVOID Dest, SIZE_T Size)
{
	KAPC_STATE  State;
	NTSTATUS	Status = STATUS_SUCCESS;
	PSIZE_T 	Bytes = 0;

	DbgPrintEx(0, 0, "\nRead Address:%p\n", SourceAddress);
	DbgPrintEx(0, 0, "Read szAddress:%x\n", Size);

	Status = MmCopyVirtualMemory(R6Process, SourceAddress, PsGetCurrentProcess(), Dest, Size, KernelMode, &Bytes);

	DbgPrintEx(0, 0, "Read Bytes:%p\n", Bytes);
	//DbgPrintEx(0, 0, "Read Output:%p\n", Dest);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Read Failed:%p\n", Status);
		return NULL;
	}
}

void Read2(DWORD64 SourceAddress, DWORD64 Dest,  SIZE_T Size)
{
	KAPC_STATE  State;
	NTSTATUS	Status = STATUS_SUCCESS;
	PSIZE_T 	Bytes = 0;
	void*		TempRead = NULL;

	DbgPrintEx(0, 0, "\nRead Address:%p\n", SourceAddress);
	DbgPrintEx(0, 0, "Read szAddress:%x\n", Size);

	Status = MmCopyVirtualMemory(R6Process, SourceAddress, PsGetCurrentProcess(), &TempRead, Size, KernelMode, &Bytes);

	DbgPrintEx(0, 0, "Read Bytes:%p\n", Bytes);
	DbgPrintEx(0, 0, "Read Output:%p\n", TempRead);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "Read Failed:%p\n", Status);
		return NULL;
	}

	Dest = TempRead;
}

NTSTATUS ReadVector3(DWORD64 SourceAddress, float* TargetVector)
{
	PSIZE_T Bytes = 0;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrintEx(0, 0, "\nReadVector Address:%p\n", SourceAddress);

	Status = MmCopyVirtualMemory(R6Process, SourceAddress, PsGetCurrentProcess(), &TargetVector[0], sizeof(float), KernelMode, &Bytes);
	Status = MmCopyVirtualMemory(R6Process, (SourceAddress + 0x4), PsGetCurrentProcess(), &TargetVector[1], sizeof(float), KernelMode, &Bytes);
	Status = MmCopyVirtualMemory(R6Process, (SourceAddress + 0x8), PsGetCurrentProcess(), &TargetVector[2], sizeof(float), KernelMode, &Bytes);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "ReadVector Failed:%p\n", Status);
		return Status;
	}

	return Status;
}

NTSTATUS ReadView(DWORD64 SourceAddress, float* TargetVector)
{
	PSIZE_T Bytes = 0;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrintEx(0, 0, "\nReadView Address:%p\n", SourceAddress);

	Status = MmCopyVirtualMemory(R6Process, SourceAddress, PsGetCurrentProcess(), &TargetVector[0], sizeof(float), KernelMode, &Bytes);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "ReadView Failed:%p\n", Status);
		return Status;
	}

	return Status;
}

DWORD64 AllocMem(DWORD64 BaseAddress, ULONG AllocType, ULONG Protection, SIZE_T Size)
{
	NTSTATUS Status = STATUS_SUCCESS;
	KAPC_STATE apc;
	ULONG old_protection;
	DWORD64 Addy = BaseAddress;

	KeStackAttachProcess(R6Process, &apc);
	Status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &Addy, 0, &Size, AllocType, Protection);
	KeUnstackDetachProcess(&apc);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "ZwAllocateVirtualMemory Failed:%p\n", Status);
		return Status;
	}

	return Addy;
}

NTSTATUS VirtualProtect(DWORD64 Address, SIZE_T Size, ULONG NewProtection)
{
	NTSTATUS Status = STATUS_SUCCESS;
	KAPC_STATE apc; ULONG OldPro = 0;
	DWORD64* pAddress = Address;

	DbgPrintEx(0, 0, "At Address: %p", pAddress);

	// Protect Address
	KeStackAttachProcess(R6Process, &apc);
	Status = ZwProtectVirtualMemory(ZwCurrentProcess(), &pAddress, &Size, NewProtection, &OldPro);
	KeUnstackDetachProcess(&apc);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "ZwProtectVirtualMemory Failed:%p\n", Status);
		return Status;
	}
	else
		DbgPrintEx(0, 0, "ZwProtectVirtualMemory Success!\n");

	return Status;
}