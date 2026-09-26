/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "CrashHandler.h"
#include "FileLogger.h"
#include "NativeNames.inc"
#include "../Menu/Menu.h"
#include "../Memory/GTAmemory.h"

#include <Windows.h>
#include <DbgHelp.h>

#include <atomic>
#include <cstdio>

#pragma comment(lib, "Dbghelp.lib")

namespace ige
{
namespace
{
constexpr INT8 kDebugLevel = static_cast<INT8>(LogType::LOG_DEBUG);
constexpr INT8 kTraceLevel = static_cast<INT8>(LogType::LOG_TRACE);
constexpr unsigned kStackFrames = 32;
constexpr unsigned kKeptNatives = 32;

// ignore noise from DBG_PRINTEXCEPTION_C/WIDE_C and THREAD_NAMING
constexpr DWORD kDbgPrintExceptionAnsi = 0x40010006;
constexpr DWORD kDbgPrintExceptionWide = 0x40010005;
constexpr DWORD kThreadNamingException = 0x406D1388;

PVOID g_vehHandle = nullptr;
std::atomic<bool> g_insideHandler{false};
bool g_symbolsReady = false;
bool g_symbolsTried = false;

unsigned long long g_recentNatives[kKeptNatives] = {};
std::atomic<unsigned> g_nativeIndex{0};
std::atomic<unsigned> g_nativeTotal{0};

const char* ExceptionName(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        return "ACCESS_VIOLATION";
    case EXCEPTION_STACK_OVERFLOW:
        return "STACK_OVERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_PRIV_INSTRUCTION:
        return "PRIV_INSTRUCTION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return "NONCONTINUABLE";
    case EXCEPTION_IN_PAGE_ERROR:
        return "IN_PAGE_ERROR";
    case EXCEPTION_GUARD_PAGE:
        return "GUARD_PAGE";
    case EXCEPTION_BREAKPOINT:
        return "BREAKPOINT";
    case EXCEPTION_SINGLE_STEP:
        return "SINGLE_STEP";
    case kDbgPrintExceptionAnsi:
        return "DBG_PRINTEXCEPTION_C";
    case kDbgPrintExceptionWide:
        return "DBG_PRINTEXCEPTION_WIDE_C";
    case kThreadNamingException:
        return "THREAD_NAMING";
    default:
        return "UNKNOWN";
    }
}

bool IsFatalException(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_GUARD_PAGE:
        return true;
    default:
        return false;
    }
}

bool IsBenignNoise(DWORD code)
{
    return code == kDbgPrintExceptionAnsi || code == kDbgPrintExceptionWide || code == kThreadNamingException;
}

// kEntries is sorted by hash; manual binary search keeps this header-free.
const char* NativeNameFor(unsigned long long hash)
{
    std::size_t lo = 0;
    std::size_t hi = NativeNames::kEntryCount;
    while (lo < hi)
    {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (NativeNames::kEntries[mid].hash < hash)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < NativeNames::kEntryCount && NativeNames::kEntries[lo].hash == hash)
        return NativeNames::kEntries[lo].name;
    return nullptr;
}

// "KERNEL32.DLL+0x1234" for known modules, raw address otherwise.
void FormatModuleOffset(const void* address, char* out, size_t outSize)
{
    HMODULE module = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCSTR>(address), &module) && module)
    {
        char path[MAX_PATH];
        if (GetModuleFileNameA(module, path, MAX_PATH))
        {
            const char* basename = strrchr(path, '\\');
            basename = basename ? basename + 1 : path;
            sprintf_s(out, outSize, "%s+0x%llX", basename, reinterpret_cast<DWORD64>(address) - reinterpret_cast<DWORD64>(module));
            return;
        }
    }
    sprintf_s(out, outSize, "0x%p", address);
}

// Export fallback names lie without a PDB, so only trust SymPdb modules.
bool TrustedSymbolFor(DWORD64 address, SYMBOL_INFO* symbol, DWORD64& displacement)
{
    if (!g_symbolsReady)
        return false;
    IMAGEHLP_MODULE64 moduleInfo = {};
    moduleInfo.SizeOfStruct = sizeof(moduleInfo);
    if (!SymGetModuleInfo64(GetCurrentProcess(), address, &moduleInfo) || moduleInfo.SymType != SymPdb)
        return false;
    displacement = 0;
    return SymFromAddr(GetCurrentProcess(), address, &displacement, symbol) == TRUE;
}

void WriteStackFrames(void** frames, unsigned frameCount)
{
    char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    for (unsigned i = 0; i < frameCount; ++i)
    {
        const DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
        char location[128];
        FormatModuleOffset(frames[i], location, sizeof(location));
        char line[512];
        DWORD64 displacement = 0;
        if (TrustedSymbolFor(address, symbol, displacement))
            sprintf_s(line, "  #%u %s %s+0x%llX", i, location, symbol->Name, displacement);
        else
            sprintf_s(line, "  #%u %s", i, location);
        myLog << line << std::endl;
    }
}

void WriteNativeHistory()
{
    const unsigned total = g_nativeTotal.load(std::memory_order_relaxed);
    const unsigned kept = total < kKeptNatives ? total : kKeptNatives;
    if (kept == 0)
    {
        myLog << "Last natives: <none recorded>" << std::endl;
        return;
    }
    myLog << "Last natives (oldest first):" << std::endl;
    const unsigned start = total - kept;
    for (unsigned i = 0; i < kept; ++i)
    {
        const unsigned long long hash = g_recentNatives[(start + i) % kKeptNatives];
        char line[128];
        if (const char* name = NativeNameFor(hash))
            sprintf_s(line, "  %s (0x%016llX)", name, hash);
        else
            sprintf_s(line, "  0x%016llX", hash);
        myLog << line << std::endl;
    }
}

// x64-only
void WriteRegisters(const CONTEXT* context)
{
    if (!context)
        return;
    char line[256];
    sprintf_s(
        line,
        "Registers: RIP=0x%llX RSP=0x%llX RBP=0x%llX RAX=0x%llX RBX=0x%llX RCX=0x%llX RDX=0x%llX",
        context->Rip,
        context->Rsp,
        context->Rbp,
        context->Rax,
        context->Rbx,
        context->Rcx,
        context->Rdx
    );
    myLog << line << std::endl;
    sprintf_s(
        line,
        "           RSI=0x%llX RDI=0x%llX R8=0x%llX R9=0x%llX R10=0x%llX R11=0x%llX R12=0x%llX R13=0x%llX R14=0x%llX R15=0x%llX EFLAGS=0x%X",
        context->Rsi,
        context->Rdi,
        context->R8,
        context->R9,
        context->R10,
        context->R11,
        context->R12,
        context->R13,
        context->R14,
        context->R15,
        context->EFlags
    );
    myLog << line << std::endl;
}

void WriteMinidump(PEXCEPTION_POINTERS exceptionInfo)
{
    HANDLE file = CreateFileA("Menyoo.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        myLog << "Minidump not written (CreateFile failed)" << std::endl;
        return;
    }
    MINIDUMP_EXCEPTION_INFORMATION exceptionParam = {};
    exceptionParam.ThreadId = GetCurrentThreadId();
    exceptionParam.ExceptionPointers = exceptionInfo;
    exceptionParam.ClientPointers = FALSE;
    const BOOL wrote = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo), &exceptionParam, nullptr, nullptr);
    CloseHandle(file);
    if (wrote)
        myLog << "Minidump written to Menyoo.dmp" << std::endl;
    else
        myLog << "Minidump not written (error " << GetLastError() << ")" << std::endl;
}

LONG CALLBACK VehHandler(PEXCEPTION_POINTERS exceptionInfo)
{
    if (g_loglevel < kDebugLevel)
        return EXCEPTION_CONTINUE_SEARCH;

    const DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;
    if (IsBenignNoise(code))
        return EXCEPTION_CONTINUE_SEARCH;
    if (!IsFatalException(code) && g_loglevel < kTraceLevel)
        return EXCEPTION_CONTINUE_SEARCH;

    if (g_insideHandler.exchange(true))
        return EXCEPTION_CONTINUE_SEARCH;

    if (!g_symbolsTried)
    {
        g_symbolsTried = true;
        HANDLE process = GetCurrentProcess();
        g_symbolsReady = SymInitialize(process, nullptr, TRUE) == TRUE;
        if (g_symbolsReady)
        {
            char exePath[MAX_PATH];
            if (GetModuleFileNameA(nullptr, exePath, MAX_PATH))
            {
                char* slash = strrchr(exePath, '\\');
                if (slash)
                    *slash = '\0';
                SymSetSearchPath(process, exePath);
            }
        }
    }

    if (myLog.is_open())
    {
        const void* faultAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
        char location[128];
        FormatModuleOffset(faultAddress, location, sizeof(location));
        char header[256];
        sprintf_s(header, "CRASH - %s (0x%08X) at %s, thread %u", ExceptionName(code), code, location, GetCurrentThreadId());
        myLog << LogType::LOG_ERROR << header << std::endl;

        const ULONG_PTR* info = exceptionInfo->ExceptionRecord->ExceptionInformation;
        if (code == EXCEPTION_ACCESS_VIOLATION && exceptionInfo->ExceptionRecord->NumberParameters >= 2)
        {
            char detail[128];
            sprintf_s(detail, "Access %s at 0x%p", info[0] == 0 ? "read" : info[0] == 1 ? "write" : "execute", reinterpret_cast<void*>(info[1]));
            myLog << detail << std::endl;
        }

        if (code == EXCEPTION_STACK_OVERFLOW)
        {
            myLog << "Stack walk skipped (stack exhausted)" << std::endl;
        }
        else
        {
            void* frames[kStackFrames];
            const unsigned frameCount = CaptureStackBackTrace(0, kStackFrames, frames, nullptr);
            WriteStackFrames(frames, frameCount);
        }

        WriteRegisters(exceptionInfo->ContextRecord);
        WriteNativeHistory();
        if (IsFatalException(code))
            WriteMinidump(exceptionInfo);
        myLog << std::flush;
    }

    g_insideHandler = false;
    return EXCEPTION_CONTINUE_SEARCH;
}
} // namespace

void InitCrashHandler()
{
    if (g_vehHandle)
        return;
    if (g_loglevel < kDebugLevel)
        return;
    g_vehHandle = AddVectoredExceptionHandler(1, VehHandler);
    if (g_vehHandle)
        addlog(LogType::LOG_DEBUG, "VEH crash handler installed");
}

void ShutdownCrashHandler()
{
    if (g_vehHandle)
    {
        RemoveVectoredExceptionHandler(g_vehHandle);
        g_vehHandle = nullptr;
    }
    if (g_symbolsReady)
    {
        SymCleanup(GetCurrentProcess());
        g_symbolsReady = false;
    }
}

void RecordNative(unsigned long long nativeHash)
{
    if (!g_vehHandle)
        return;
    const unsigned slot = g_nativeIndex.fetch_add(1, std::memory_order_relaxed) % kKeptNatives;
    g_recentNatives[slot] = nativeHash;
    g_nativeTotal.fetch_add(1, std::memory_order_relaxed);
}
} // namespace ige
