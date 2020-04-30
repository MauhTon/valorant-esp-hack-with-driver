#pragma once
#include <ntifs.h>

// Shared Memory Functions
NTSTATUS CreateShareMemory();
NTSTATUS ReadSharedMemory();
VOID CloseMemory();

// Shared Memory Vars
extern HANDLE  g_Section;
extern PVOID   g_SharedSection;

// Game Vars
extern DWORD64		KernelBase;
extern DWORD64		BaseAddrR6;
extern PEPROCESS	R6Process;
extern ULONG		PID_R6;

// Memory Control Functions
NTSTATUS Write(ULONG pid, DWORD64 SourceAddress, PVOID TargetAddress, SIZE_T Size);
NTSTATUS WriteByte(DWORD64 SourceAddress, unsigned char TargetAddress, SIZE_T Size);
NTSTATUS WriteFloat(DWORD64 SourceAddress, float* TargetAddress, SIZE_T Size);
NTSTATUS WriteInt(DWORD64 SourceAddress, int TargetAddress, SIZE_T Size);
void     Read(DWORD64 SourceAddress, PVOID Dest, SIZE_T Size);
void     Read2(DWORD64 SourceAddress, DWORD64 Dest, SIZE_T Size);
NTSTATUS ReadVector3(DWORD64 SourceAddress, float* TargetVector);
NTSTATUS ReadView(DWORD64 SourceAddress, float* TargetVector);
DWORD64  AllocMem(DWORD64 BaseAddress, ULONG AllocType, ULONG Protection, SIZE_T Size);
NTSTATUS VirtualProtect(DWORD64 Address, SIZE_T Size, ULONG NewProtection);