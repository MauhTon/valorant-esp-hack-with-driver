#include <chrono>
#include <iomanip>
#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <sstream>
#include <vector>

#define INRANGE(x,a,b)   (x >= a && x <= b)
#define GET_BYTE( x )    (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))
#define GET_BITS( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define PTRMAXVAL ((PVOID)0x000F000000000000)

static void GetCurrentSystemTime(tm& timeInfo)
{
	const std::chrono::system_clock::time_point systemNow = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(systemNow);
	localtime_s(&timeInfo, &now_c); // using localtime_s as std::localtime is not thread-safe.
};

// basefunct
static std::string SetupStringParams(std::string szBasicString)
{
	return szBasicString;
}

// Replace % with a desired string / value represented after semicolons. Works kinda like printf.
template <typename T, typename... Targs>
static std::string SetupStringParams(std::string szBasicString, T arg, Targs&& ...args)
{
	const auto found = szBasicString.find_first_of('%');
	if (found != std::string::npos)
	{
		std::stringstream tmp;
		tmp << arg;
		szBasicString.replace(found, 1, tmp.str());
		szBasicString = SetupStringParams(szBasicString, std::forward<Targs>(args)...);
	}
	return szBasicString;
}

namespace Utils
{
	template <typename ... Args>
	static void Log(const std::string& str, Args ...arguments)
	{
		Log(SetupStringParams(str.c_str(), arguments...));
	}

	static void Log(const std::string& str)
	{
		tm timeInfo{ };
		GetCurrentSystemTime(timeInfo);

		std::stringstream ssTime; // Temp stringstream to keep things clean
		ssTime << "[" << std::put_time(&timeInfo, "%T") << "] " << str << std::endl;

		std::cout << ssTime.str();
	};

	template<class T>
	T Read(const DWORD64 dwPtr)
	{
		if (!IsBadReadPtr((void*)dwPtr, sizeof(T)))
			return *(T*)dwPtr;
		return 0;
	}
	static __forceinline BOOLEAN IsValid(PVOID ptr)
	{
		return (ptr >= (PVOID)0x10000) && (ptr < PTRMAXVAL) && ptr != nullptr && !IsBadReadPtr(ptr, sizeof(ptr));
	}
	template <class vType>
	static BOOLEAN Write(uintptr_t address, vType value)
	{
		if (IsValid((vType*)(address)))
		{
			*(vType*)(address) = value;
			return TRUE;
		}
		return FALSE;
	}

	template <typename vType>
	static vType ReadPtr(std::initializer_list<uintptr_t> _Offsets, bool ReadFirstOffset)
	{
		uintptr_t LastPtr = NULL;
		int OffsetsSize = NULL;
		std::vector<uintptr_t> Offsets = { NULL };
		Offsets = _Offsets;
		OffsetsSize = Offsets.size();
		LastPtr = Read<uintptr_t>((ReadFirstOffset ? Read<uintptr_t>(Offsets[0]) : Offsets[0]) + Offsets[1]);
		for (size_t i = 2; i < OffsetsSize - 1; i++)
			if (!(LastPtr = Read<uintptr_t>(LastPtr + Offsets[i])))
				return vType();
		return Read<vType>(LastPtr + Offsets[OffsetsSize - 1]);
	}

	template <typename vType>
	static BOOLEAN WritePtr(std::initializer_list<uintptr_t> _Offsets, vType _value, bool ReadFirstOffset)
	{
		uintptr_t LastPtr = NULL;
		int OffsetsSize = NULL;
		std::vector<uintptr_t> Offsets = { NULL };
		Offsets = _Offsets;
		OffsetsSize = Offsets.size();
		LastPtr = Read<uintptr_t>((ReadFirstOffset ? Read<uintptr_t>(Offsets[0]) : Offsets[0]) + Offsets[1]);
		for (size_t i = 2; i < OffsetsSize - 1; i++)
			if (!(LastPtr = Read<uintptr_t>(LastPtr + Offsets[i])))
				return FALSE;
		return Write<vType>(LastPtr + Offsets[OffsetsSize - 1], _value);
	}

	static uint8_t* FindSignature(uintptr_t Module, const char* szSignature)
	{
		static auto pattern_to_byte = [](const char* pattern) {
			auto bytes = std::vector < int >{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + strlen(pattern);

			for (auto current = start; current < end; ++current) {
				if (*current == '?') {
					++current;
					if (*current == '?')
						++current;
					bytes.push_back(-1);
				}
				else {
					bytes.push_back(strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

		auto dosHeader = (PIMAGE_DOS_HEADER)Module;
		auto ntHeaders = (PIMAGE_NT_HEADERS)((uint8_t*)Module + dosHeader->e_lfanew);

		auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		auto patternBytes = pattern_to_byte(szSignature);
		auto scanBytes = reinterpret_cast<uint8_t*>(Module);

		auto s = patternBytes.size();
		auto d = patternBytes.data();

		for (auto i = 0ul; i < sizeOfImage - s; ++i) {
			bool found = true;
			for (auto j = 0ul; j < s; ++j) {
				if (scanBytes[i + j] != d[j] && d[j] != -1) {
					found = false;
					break;
				}
			}
			if (found) {
				return &scanBytes[i];
			}
		}
		return nullptr;
	}
}