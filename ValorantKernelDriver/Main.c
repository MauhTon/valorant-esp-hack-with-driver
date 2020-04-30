#include <ntstatus.h>
#include "Clean.h"
#include "Dependencies.h"
#include "Helpers.h"
#include "Memory.h"

#pragma comment(linker, "/ENTRY:\"RealEntryPoint\"")
static LARGE_INTEGER Timeout;

NTKERNELAPI PVOID PsGetProcessSectionBaseAddress(__in PEPROCESS Process);
NTSTATUS GetImageBase(PEPROCESS Process)
{
	KAPC_STATE State;

	KeStackAttachProcess(Process, &State);
	BaseAddrR6 = (DWORD64*)PsGetProcessSectionBaseAddress(Process);
	KeUnstackDetachProcess(&State);
	DbgPrintEx(0, 0, "Image Found:%p\n", BaseAddrR6);
	return STATUS_SUCCESS;
}

ULONGLONG GetModuleHandle(UNICODE_STRING module_name)
{
	PPEB pPeb = PsGetProcessPeb(R6Process);

	if (!pPeb) 
		return 0; // Fucking kill your self retard 

	KAPC_STATE state;

	KeStackAttachProcess(R6Process, &state);

	PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;

	if (!pLdr) 
	{
		KeUnstackDetachProcess(&state);
		return 0; // failed
	}

	// loop the linked list
	for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink;
		list != &pLdr->ModuleListLoadOrder; list = (PLIST_ENTRY)list->Flink) 
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
		if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) == 0) 
		{
			ULONG64 baseAddr = (ULONG64)pEntry->DllBase;
			KeUnstackDetachProcess(&state);
			return baseAddr;
		}
	}
	KeUnstackDetachProcess(&state);

	return 0;
}

NTSTATUS GetPid()
{
	// ZwQuery
	ULONG CallBackLength = 0;
	PSYSTEM_PROCESS_INFO PSI = NULL;
	PSYSTEM_PROCESS_INFO pCurrent = NULL;
	PVOID BufferPid = NULL;

	// Names
	PCWSTR R6 = L"RainbowSix.exe";
	UNICODE_STRING uImageNameR6;
	RtlInitUnicodeString(&uImageNameR6, R6);

	if (!NT_SUCCESS(ZwQuerySystemInformation(SystemProcessInformation, NULL, NULL, &CallBackLength)))
	{
		BufferPid = ExAllocatePoolWithTag(NonPagedPool, CallBackLength, 0x616b7963); // aykc 
		if (!BufferPid)
		{
			DbgPrintEx(0, 0, "Failed To Allocate Buffer Notify Routine");
			return STATUS_UNSUCCESSFUL;
		}

		PSI = (PSYSTEM_PROCESS_INFO)BufferPid;
		if (!NT_SUCCESS(ZwQuerySystemInformation(SystemProcessInformation, PSI, CallBackLength, NULL)))
		{
			DbgPrintEx(0, 0, "Failed To Get Query System Process Information List");
			ExFreePoolWithTag(BufferPid, 0x616b7963);
			return STATUS_UNSUCCESSFUL;
		}

		DbgPrintEx(0, 0, "\nSearching For PID...\n\n");
		do
		{
			if (PSI->NextEntryOffset == 0)
				break;

			if (RtlEqualUnicodeString(&uImageNameR6, &PSI->ImageName, FALSE))
			{
				DbgPrintEx(0, 0, "PID %d | NAME %ws", PSI->ProcessId, PSI->ImageName.Buffer);
				PID_R6 = PSI->ProcessId;
				break;
			}

			PSI = (PSYSTEM_PROCESS_INFO)((unsigned char*)PSI + PSI->NextEntryOffset); // Calculate the address of the next entry.

		} while (PSI->NextEntryOffset);

		// Free Allocated Memory
		ExFreePoolWithTag(BufferPid, 0x616b7963);
	}

	return STATUS_SUCCESS;
}

NTSTATUS FindGamePID()
{
	NTSTATUS Status = STATUS_SUCCESS;

	if (!PID_R6)
	{
		while (TRUE)
		{
			//Wait 1 Secound
			Timeout.QuadPart = RELATIVE(SECONDS(1));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get PID of Rainbow
			if (!NT_SUCCESS(GetPid()))
			{
				DbgPrintEx(0, 0, "Get PID Failed:\n");
				return;
			}

			Status = ReadSharedMemory();
			if (!NT_SUCCESS(Status))
			{
				DbgPrintEx(0, 0, "Read Shared Memory Failed:%p\n", Status);
				break;
			}

			if (strcmp((PCHAR)g_SharedSection, "Stop") == 0)
			{
				DbgPrint("\nMain Loop Ending...\n");
				break;
			}

			if (PID_R6)
				break;
		}
	}

	if (PID_R6)
	{
		Status = PsLookupProcessByProcessId((HANDLE)PID_R6, &R6Process);
		if (!NT_SUCCESS(Status))
		{
			DbgPrintEx(0, 0, "PsLookupProcessByProcessId Failed (Rainbow 6 PID):\n", Status);
			return Status;
		}
	}

	return Status;
}

NTSTATUS FindGameBase()
{
	if (R6Process)
		GetImageBase(R6Process);

	return STATUS_SUCCESS;
}

VOID DriverLoop()
{
	NTSTATUS Status = STATUS_SUCCESS;
	DbgPrintEx(0, 0, "\nMain Driver Loop Started");

	if (!NT_SUCCESS(CreateShareMemory()))
	{
		DbgPrintEx(0, 0, "CreateShareMemory Failed:\n");
		return;
	}

	while (TRUE)
	{
		/* Begin Checking For Newly Copied Strings */
		Status = ReadSharedMemory();
		if (!NT_SUCCESS(Status))
		{
			DbgPrintEx(0, 0, "Read Shared Memory Failed:%p\n", Status);
			break;
		}

		if (strcmp((PCHAR)g_SharedSection, "Stop") == 0)
		{
			DbgPrint("\nMain Loop Ending...\n");
			break;
		}

		if (strcmp((PCHAR)g_SharedSection, "START") == 0)
		{
			if (!NT_SUCCESS(FindGamePID()))
			{
				DbgPrintEx(0, 0, "FindGamePID Failed\n");
				/* Clean Up */
				CloseMemory();
				break;
			}

			if (!NT_SUCCESS(FindGameBase()))
			{
				DbgPrintEx(0, 0, "FindGameBase Failed\n");
				/* Clean Up */
				CloseMemory();
				if (R6Process)
					ObDereferenceObject(R6Process);

				break;
			}
		}

		while (!(PCHAR)g_SharedSection == NULL && strcmp((PCHAR)g_SharedSection, "Mod") == 0)
		{
			// Wait For New Input
			Timeout.QuadPart = RELATIVE(MILLISECONDS(12));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get User Input
			ReadSharedMemory();
			Get_Module* ReadRecived = (Get_Module*)g_SharedSection;

			ANSI_STRING AS;
			UNICODE_STRING ModuleNAme;

			RtlInitAnsiString(&AS, ReadRecived->m_pModName);
			RtlAnsiStringToUnicodeString(&ModuleNAme, &AS, TRUE);
			
			DbgPrintEx(0, 0, "\nModule Name: %wZ", ModuleNAme);

			ReadRecived->m_ulModBase = GetModuleHandle(ModuleNAme);

			DbgPrintEx(0, 0, "Module Base: %p\n", ReadRecived->m_ulModBase);

			// Send Back Returned Read Address
			ReadSharedMemory();
			if (memcpy(g_SharedSection, ReadRecived, sizeof(Get_Module)) == 0)
				DbgPrintEx(0, 0, "Sending Read Address Back Failed\n");

			break;
		}	

		while (!(PCHAR)g_SharedSection == NULL && strcmp((PCHAR)g_SharedSection, "Read") == 0)
		{
			// Wait For New Input
			Timeout.QuadPart = RELATIVE(MILLISECONDS(12));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get User Input
			ReadSharedMemory();
			_Read* ReadRecived = (_Read*)g_SharedSection;

			if (ReadRecived->src)
			{
				void* Temp = NULL;
				Read(ReadRecived->src, &Temp, ReadRecived->size);
				ReadRecived->dst = Temp;
			}

			// Send Back Returned Read Address
			ReadSharedMemory();
			if (memcpy(g_SharedSection, ReadRecived, sizeof(_Read)) == 0)
				DbgPrintEx(0, 0, "Sending Read Address Back Failed\n");

			break;
		}			

		while (!(PCHAR)g_SharedSection == NULL && strcmp((PCHAR)g_SharedSection, "Readd") == 0)
		{
			// Wait For New Input
			Timeout.QuadPart = RELATIVE(MILLISECONDS(12));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get User Input
			ReadSharedMemory();
			_Read* ReadRecived = (_Read*)g_SharedSection;

			PVOID pBuffer = ExAllocatePool(NonPagedPool, ReadRecived->size);
			if (ReadRecived->src)
			{
				Read(ReadRecived->src, &pBuffer, ReadRecived->size);
				ReadRecived->dst = pBuffer;
			}
			
			// Send Back Returned Read Address
			ReadSharedMemory();
			if (memcpy(g_SharedSection, ReadRecived, sizeof(_Read)) == 0)
				DbgPrintEx(0, 0, "Sending Read Address Back Failed\n");

			ExFreePool(pBuffer);

			break;
		}		

		while (!(PCHAR)g_SharedSection == NULL && strcmp((PCHAR)g_SharedSection, "PE") == 0)
		{
			// Wait For New Input
			Timeout.QuadPart = RELATIVE(MILLISECONDS(10));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get User Input
			ReadSharedMemory();
			_WritePe* ReadRecived = (_WritePe*)g_SharedSection;

			if (ReadRecived->size && ReadRecived->dst)
				Write(ReadRecived->pid, ReadRecived->dst, ReadRecived->src, ReadRecived->size);		

			break;
		}

		while (!(PCHAR)g_SharedSection == NULL && strcmp((PCHAR)g_SharedSection, "Write") == 0)
		{
			// Wait For New Input
			Timeout.QuadPart = RELATIVE(MILLISECONDS(10));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get User Input
			ReadSharedMemory();
			_Write* ReadRecived = (_Write*)g_SharedSection;

			if (ReadRecived->size && ReadRecived->dst)
				Write(ReadRecived->pid, ReadRecived->dst, ReadRecived->src, ReadRecived->size);		

			break;
		}

		while (!(PCHAR)g_SharedSection == NULL && strcmp((PCHAR)g_SharedSection, "All") == 0)
		{
			// Wait For New Input
			Timeout.QuadPart = RELATIVE(MILLISECONDS(12));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get User Input
			ReadSharedMemory();
			Alloc_Mem* ReadRecived = (Alloc_Mem*)g_SharedSection;

			DbgPrintEx(0, 0, "\nAllocating Memory...");

			ReadRecived->dst = AllocMem(ReadRecived->addr, ReadRecived->allocation_type, ReadRecived->protect, ReadRecived->size);

			DbgPrintEx(0, 0, "Finished at: %p\n", ReadRecived->dst);

			// Send Back Returned Read Address
			ReadSharedMemory();
			if (memcpy(g_SharedSection, ReadRecived, sizeof(Alloc_Mem)) == 0)
				DbgPrintEx(0, 0, "Sending Read Address Back Failed\n");

			break;
		}			

		while (!(PCHAR)g_SharedSection == NULL && strcmp((PCHAR)g_SharedSection, "Pro") == 0)
		{
			// Wait For New Input
			Timeout.QuadPart = RELATIVE(MILLISECONDS(12));
			KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

			// Get User Input
			ReadSharedMemory();
			Protect_Mem* ReadRecived = (Protect_Mem*)g_SharedSection;

			DbgPrintEx(0, 0, "\nProtecting Memory with protection: %x", ReadRecived->protect);

			VirtualProtect(ReadRecived->addr, ReadRecived->size, ReadRecived->protect);

			break;
		}		

		Timeout.QuadPart = RELATIVE(MILLISECONDS(1));
		KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
	}

End:;

	/* Clean Up */
	CloseMemory();

	if (R6Process)
		ObDereferenceObject(R6Process);

	return;
}

NTSTATUS RealEntryPoint()
{
	DbgPrintEx(0, 0, "\nReal Entry Called.");

	// Vars
	NTSTATUS Status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES ObjAtt = { 0 };
	HANDLE hThread = NULL;

	// Clean
	CleanPiDDB();

	// Setup Our Thread
	// TODO: Replace This With Something Like A Hook Idk
	// Not Reliable, Probaly Detected On EAC, Maybe Not BE
	// https://www.unknowncheats.me/forum/anti-cheat-bypass/322618-socket-driver.html
	// https://www.unknowncheats.me/forum/anti-cheat-bypass/325212-eac-system-thread-detection.html
	InitializeObjectAttributes(&ObjAtt, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	Status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &ObjAtt, NULL, NULL, DriverLoop, NULL);
	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "PsCreateSystemThread Failed:\n", Status);
		return Status;
	}

	return Status;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObj, _In_ PUNICODE_STRING RegistryPath)
{
	DbgPrintEx(0, 0, "Driver Created.\n");

	// Fix Paramms
	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(DriverObj);

	return STATUS_SUCCESS;
}