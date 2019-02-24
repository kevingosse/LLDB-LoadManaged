// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <cstdarg>
#include "mstypes.h"
#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBCommandInterpreter.h"
#include "lldb/API/SBCommandReturnObject.h"

#define DBG_TARGET_AMD64

#define STDMETHODCALLTYPE

#define DT_SIZE_OF_80387_REGISTERS      80
#define DT_MAXIMUM_SUPPORTED_EXTENSION     512

typedef struct {
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    BYTE    RegisterArea[DT_SIZE_OF_80387_REGISTERS];
    DWORD   Cr0NpxState;
} DT_FLOATING_SAVE_AREA;

#define DT_CONTEXT_AMD64            0x00100000L

#define DT_CONTEXT_CONTROL          (DT_CONTEXT_AMD64 | 0x00000001L)
#define DT_CONTEXT_INTEGER          (DT_CONTEXT_AMD64 | 0x00000002L)
#define DT_CONTEXT_SEGMENTS         (DT_CONTEXT_AMD64 | 0x00000004L)
#define DT_CONTEXT_FLOATING_POINT   (DT_CONTEXT_AMD64 | 0x00000008L)
#define DT_CONTEXT_DEBUG_REGISTERS  (DT_CONTEXT_AMD64 | 0x00000010L)

#define DT_CONTEXT_FULL (DT_CONTEXT_CONTROL | DT_CONTEXT_INTEGER | DT_CONTEXT_FLOATING_POINT)
#define DT_CONTEXT_ALL (DT_CONTEXT_CONTROL | DT_CONTEXT_INTEGER | DT_CONTEXT_SEGMENTS | DT_CONTEXT_FLOATING_POINT | DT_CONTEXT_DEBUG_REGISTERS)

typedef struct  {
    ULONGLONG Low;
    LONGLONG High;
} DT_M128A;

typedef struct  {
    WORD   ControlWord;
    WORD   StatusWord;
    BYTE  TagWord;
    BYTE  Reserved1;
    WORD   ErrorOpcode;
    DWORD ErrorOffset;
    WORD   ErrorSelector;
    WORD   Reserved2;
    DWORD DataOffset;
    WORD   DataSelector;
    WORD   Reserved3;
    DWORD MxCsr;
    DWORD MxCsr_Mask;
    DT_M128A FloatRegisters[8];
    DT_M128A XmmRegisters[16];
    BYTE  Reserved4[96];
} DT_XMM_SAVE_AREA32;

#define DECLSPEC_ALIGN(x)

typedef struct DECLSPEC_ALIGN(16) {

DWORD64 P1Home;
DWORD64 P2Home;
DWORD64 P3Home;
DWORD64 P4Home;
DWORD64 P5Home;
DWORD64 P6Home;

DWORD ContextFlags;
DWORD MxCsr;

WORD   SegCs;
WORD   SegDs;
WORD   SegEs;
WORD   SegFs;
WORD   SegGs;
WORD   SegSs;
DWORD EFlags;

DWORD64 Dr0;
DWORD64 Dr1;
DWORD64 Dr2;
DWORD64 Dr3;
DWORD64 Dr6;
DWORD64 Dr7;

DWORD64 Rax;
DWORD64 Rcx;
DWORD64 Rdx;
DWORD64 Rbx;
DWORD64 Rsp;
DWORD64 Rbp;
DWORD64 Rsi;
DWORD64 Rdi;
DWORD64 R8;
DWORD64 R9;
DWORD64 R10;
DWORD64 R11;
DWORD64 R12;
DWORD64 R13;
DWORD64 R14;
DWORD64 R15;

DWORD64 Rip;

union {
    DT_XMM_SAVE_AREA32 FltSave;
    struct {
        DT_M128A Header[2];
        DT_M128A Legacy[8];
        DT_M128A Xmm0;
        DT_M128A Xmm1;
        DT_M128A Xmm2;
        DT_M128A Xmm3;
        DT_M128A Xmm4;
        DT_M128A Xmm5;
        DT_M128A Xmm6;
        DT_M128A Xmm7;
        DT_M128A Xmm8;
        DT_M128A Xmm9;
        DT_M128A Xmm10;
        DT_M128A Xmm11;
        DT_M128A Xmm12;
        DT_M128A Xmm13;
        DT_M128A Xmm14;
        DT_M128A Xmm15;
    };
};

DT_M128A VectorRegister[26];
DWORD64 VectorControl;

DWORD64 DebugControl;
DWORD64 LastBranchToRip;
DWORD64 LastBranchFromRip;
DWORD64 LastExceptionToRip;
DWORD64 LastExceptionFromRip;
} DT_CONTEXT;


class LLDBServices : public ILLDBServices
{
private:
    LONG m_ref;
    lldb::SBDebugger &m_debugger;
    lldb::SBCommandReturnObject &m_returnObject;

    lldb::SBProcess *m_currentProcess;
    lldb::SBThread *m_currentThread;

    void OutputString(ULONG mask, PCSTR str);
    ULONG64 GetModuleBase(lldb::SBTarget& target, lldb::SBModule& module);
    DWORD_PTR GetExpression(lldb::SBFrame& frame, lldb::SBError& error, PCSTR exp);
    void GetContextFromFrame(lldb::SBFrame& frame, DT_CONTEXT *dtcontext);
    DWORD_PTR GetRegister(lldb::SBFrame& frame, const char *name);

    lldb::SBProcess GetCurrentProcess();
    lldb::SBThread GetCurrentThread();
    lldb::SBFrame GetCurrentFrame();

public:
    LLDBServices(lldb::SBDebugger &debugger, lldb::SBCommandReturnObject &returnObject, lldb::SBProcess *process = nullptr, lldb::SBThread *thread = nullptr);
    ~LLDBServices();

    //----------------------------------------------------------------------------
    // IUnknown
    //----------------------------------------------------------------------------

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        PVOID* Interface);

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //----------------------------------------------------------------------------
    // ILLDBServices
    //----------------------------------------------------------------------------

    virtual PCSTR GetCoreClrDirectory();

    virtual DWORD_PTR GetExpression(
        PCSTR exp);

    virtual HRESULT VirtualUnwind(
        DWORD threadID,
        ULONG32 contextSize,
        PBYTE context);

    virtual HRESULT SetExceptionCallback(
        PFN_EXCEPTION_CALLBACK callback);

    virtual HRESULT ClearExceptionCallback();

    //----------------------------------------------------------------------------
    // IDebugControl2
    //----------------------------------------------------------------------------

    virtual HRESULT GetInterrupt();

    virtual HRESULT Output(
        ULONG mask,
        PCSTR format,
        ...);

    virtual HRESULT OutputVaList(
        ULONG mask,
        PCSTR format,
        va_list args);

    virtual HRESULT ControlledOutput(
        ULONG outputControl,
        ULONG mask,
        PCSTR format,
        ...);

    virtual HRESULT ControlledOutputVaList(
        ULONG outputControl,
        ULONG mask,
        PCSTR format,
        va_list args);

    virtual HRESULT GetDebuggeeType(
        PULONG debugClass,
        PULONG qualifier);

    virtual HRESULT GetPageSize(
        PULONG size);

    virtual HRESULT GetExecutingProcessorType(
        PULONG type);

    virtual HRESULT Execute(
        ULONG outputControl,
        PCSTR command,
        ULONG flags);

    virtual HRESULT GetLastEventInformation(
        PULONG type,
        PULONG processId,
        PULONG threadId,
        PVOID extraInformation,
        ULONG extraInformationSize,
        PULONG extraInformationUsed,
        PSTR description,
        ULONG descriptionSize,
        PULONG descriptionUsed);

    virtual HRESULT Disassemble(
        ULONG64 offset,
        ULONG flags,
        PSTR buffer,
        ULONG bufferSize,
        PULONG disassemblySize,
        PULONG64 endOffset);

    //----------------------------------------------------------------------------
    // IDebugControl4
    //----------------------------------------------------------------------------

    virtual HRESULT
    GetContextStackTrace(
        PVOID startContext,
        ULONG startContextSize,
        PDEBUG_STACK_FRAME frames,
        ULONG framesSize,
        PVOID frameContexts,
        ULONG frameContextsSize,
        ULONG frameContextsEntrySize,
        PULONG framesFilled);

    //----------------------------------------------------------------------------
    // IDebugDataSpaces
    //----------------------------------------------------------------------------

    virtual HRESULT ReadVirtual(
        ULONG64 offset,
        PVOID buffer,
        ULONG bufferSize,
        PULONG bytesRead);

    virtual HRESULT WriteVirtual(
        ULONG64 offset,
        PVOID buffer,
        ULONG bufferSize,
        PULONG bytesWritten);

    //----------------------------------------------------------------------------
    // IDebugSymbols
    //----------------------------------------------------------------------------

    virtual HRESULT GetSymbolOptions(
        PULONG options);

    virtual HRESULT GetNameByOffset(
        ULONG64 offset,
        PSTR nameBuffer,
        ULONG nameBufferSize,
        PULONG nameSize,
        PULONG64 displacement);

    virtual HRESULT GetNumberModules(
        PULONG loaded,
        PULONG unloaded);

    virtual HRESULT GetModuleByIndex(
        ULONG index,
        PULONG64 base);

    virtual HRESULT GetModuleByModuleName(
        PCSTR name,
        ULONG startIndex,
        PULONG index,
        PULONG64 base);

    virtual HRESULT GetModuleByOffset(
        ULONG64 offset,
        ULONG startIndex,
        PULONG index,
        PULONG64 base);

    virtual HRESULT GetModuleNames(
        ULONG index,
        ULONG64 base,
        PSTR imageNameBuffer,
        ULONG imageNameBufferSize,
        PULONG imageNameSize,
        PSTR moduleNameBuffer,
        ULONG moduleNameBufferSize,
        PULONG moduleNameSize,
        PSTR loadedImageNameBuffer,
        ULONG loadedImageNameBufferSize,
        PULONG loadedImageNameSize);

    virtual HRESULT GetModuleParameters(
        ULONG count,
        PULONG64 bases,
        ULONG start,
        PDEBUG_MODULE_PARAMETERS params
    );

    virtual HRESULT GetModuleNameString(
            ULONG   Which,
            ULONG   Index,
            ULONG64 Base,
            PSTR    Buffer,
            ULONG   BufferSize,
            PULONG  NameSize
    );

    virtual HRESULT IsPointer64Bit();

    virtual HRESULT GetLineByOffset(
        ULONG64 offset,
        PULONG line,
        PSTR fileBuffer,
        ULONG fileBufferSize,
        PULONG fileSize,
        PULONG64 displacement);

    virtual HRESULT GetSourceFileLineOffsets(
        PCSTR file,
        PULONG64 buffer,
        ULONG bufferLines,
        PULONG fileLines);

    virtual HRESULT FindSourceFile(
        ULONG startElement,
        PCSTR file,
        ULONG flags,
        PULONG foundElement,
        PSTR buffer,
        ULONG bufferSize,
        PULONG foundSize);

    //----------------------------------------------------------------------------
    // IDebugSystemObjects
    //----------------------------------------------------------------------------

    virtual HRESULT GetCurrentProcessId(
        PULONG id);

    virtual HRESULT GetCurrentThreadId(
        PULONG id);

    virtual HRESULT SetCurrentThreadId(
        ULONG id);

    virtual HRESULT GetCurrentThreadSystemId(
        PULONG sysId);

    virtual HRESULT GetThreadIdBySystemId(
        ULONG sysId,
        PULONG threadId);

    virtual HRESULT GetThreadContextById(
        ULONG32 threadID,
        ULONG32 contextFlags,
        ULONG32 contextSize,
        PBYTE context);

    //----------------------------------------------------------------------------
    // IDebugRegisters
    //----------------------------------------------------------------------------

    virtual HRESULT GetValueByName(
        PCSTR name,
        PDWORD_PTR debugValue);

    virtual HRESULT GetInstructionOffset(
        PULONG64 offset);

    virtual HRESULT GetStackOffset(
        PULONG64 offset);

    virtual HRESULT GetFrameOffset(
        PULONG64 offset);

    //----------------------------------------------------------------------------
    // LLDBServices (internal)
    //----------------------------------------------------------------------------

    virtual PCSTR GetModuleDirectory(
        PCSTR name);
};