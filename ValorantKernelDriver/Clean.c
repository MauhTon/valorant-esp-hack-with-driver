#include "Clean.h"
#include "Helpers.h"
#include <ntdef.h>
#include <ntifs.h>

static uintptr_t get_kernel_address(const char* name, size_t* size)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG neededSize = 0;

	ZwQuerySystemInformation(
		SystemModuleInformation,
		&neededSize,
		0,
		&neededSize
	);

	PSYSTEM_MODULE_INFORMATION pModuleList;

	pModuleList = (PSYSTEM_MODULE_INFORMATION)ExAllocatePool(NonPagedPool, neededSize);

	if (!pModuleList)
	{
		log("ExAllocatePoolWithTag failed(kernel addr)\n");
		return 0;
	}

	status = ZwQuerySystemInformation(SystemModuleInformation,
		pModuleList,
		neededSize,
		0
	);

	ULONG i = 0;
	uintptr_t address = 0;

	for (i = 0; i < pModuleList->ulModuleCount; i++)
	{
		SYSTEM_MODULE mod = pModuleList->Modules[i];

		address = (uintptr_t)(pModuleList->Modules[i].Base);
		*size = (uintptr_t)(pModuleList->Modules[i].Size);
		if (strstr(mod.ImageName, name) != NULL)
			break;
	}

	ExFreePool(pModuleList);

	return address;
}


unsigned __int64 dereference(unsigned __int64 address, unsigned int offset)
{
	if (address == 0)
		return 0;

	return address + (int)((*(int*)(address + offset) + offset) + sizeof(int));
}

unsigned __int64 find_pattern(void* start, size_t length, const char* pattern, const char* mask)
{
	const char* data = (const char*)start;
	const auto pattern_length = strlen(mask);

	for (size_t i = 0; i <= length - pattern_length; i++)
	{
		BOOLEAN accumulative_found = TRUE;

		for (size_t j = 0; j < pattern_length; j++)
		{
			if (!MmIsAddressValid((void*)((unsigned __int64)data + i + j)))
			{
				accumulative_found = FALSE;
				break;
			}

			if (data[i + j] != pattern[j] && mask[j] != '?')
			{
				accumulative_found = FALSE;
				break;
			}
		}

		if (accumulative_found)
		{
			return (unsigned __int64)((unsigned __int64)data + i);
		}
	}

	return NULL;
}

NTSTATUS CleanPiDDB()
{
	PCWSTR pDrivNam = L"monitor.sys";
	UNICODE_STRING DrivNam;
	RtlInitUnicodeString(&DrivNam, pDrivNam);

#ifdef _DEBUG
	DbgPrintEx(0, 0, "\nCleanPiDDB started!");
#endif
	PRTL_AVL_TABLE PiDDBCacheTable;

	size_t size;
	uintptr_t ntoskrnlBase = get_kernel_address("ntoskrnl.exe", &size);

	//DbgPrintEx(0, 0, "ntoskrnl.exe: %d\n", ntoskrnlBase);
	//DbgPrintEx(0, 0, "ntoskrnl.exe size: %d\n", size);

	PiDDBCacheTable = (PRTL_AVL_TABLE)dereference(find_pattern((void*)ntoskrnlBase, size, "\x48\x8d\x0d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x3d\x00\x00\x00\x00\x0f\x83", "xxx????x????x????xx"), 3);

	//DbgPrintEx(0, 0, "PiDDBCacheTable: %d\n", PiDDBCacheTable);

	if (!PiDDBCacheTable)
	{
#ifdef _DEBUG
		log("PiDDBCacheTable NULL\n");
#endif
		return 0;
	}

	uintptr_t entry_address = (uintptr_t)(PiDDBCacheTable->BalancedRoot.RightChild) + sizeof(RTL_BALANCED_LINKS);
	//DbgPrintEx(0, 0, "entry_address: %d\n", entry_address);

	piddbcache* entry = (piddbcache*)(entry_address);

	/*capcom.sys(drvmap) : 0x57CD1415 iqvw64e.sys(kdmapper) : 0x5284EAC3, also cpuz driver*/
	if (entry->TimeDateStamp == 0x57CD1415 || entry->TimeDateStamp == 0x5284EAC3)
	{
		entry->TimeDateStamp = 0x54EAC3;
		entry->DriverName = DrivNam;
	}

	ULONG count = 0;
	for (PLIST_ENTRY link = entry->List.Flink; link != entry->List.Blink; link = link->Flink, count++)
	{
		piddbcache* cache_entry = (piddbcache*)(link);
		if (cache_entry->TimeDateStamp == 0x57CD1415 || cache_entry->TimeDateStamp == 0x5284EAC3)
		{
#ifdef _DEBUG
			DbgPrintEx(0, 0, "\nCacheEntry Count: %lu Name: %wZ \t\t Stamp: %x\n",
				count,
				cache_entry->DriverName,
				cache_entry->TimeDateStamp);
#endif

			cache_entry->TimeDateStamp = 0x54EAC4 + count;
			cache_entry->DriverName = DrivNam;
#ifdef _DEBUG
			DbgPrintEx(0, 0, "CacheEntry Count: %lu Name: %wZ \t\t Stamp: %x\n",
				count,
				cache_entry->DriverName,
				cache_entry->TimeDateStamp);
#endif
		}
	}
}
