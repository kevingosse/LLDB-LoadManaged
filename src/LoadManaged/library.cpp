#include "library.h"

#include <iostream>
#include <cstdio>
#include "coreruncommon.h"
#include "services.h"
#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBCommandInterpreter.h"
#include "lldb/API/SBCommandReturnObject.h"
#include "lldb/API/SBTarget.h"
#include <link.h>
#include <libgen.h>
#include "ClrInterop.cpp"

namespace lldb
{
    bool PluginInitialize(lldb::SBDebugger debugger);
}

char* libraryPath;
const char* clrPath = "/usr/share/dotnet/shared/Microsoft.NETCore.App/2.2.1/";

class SetClrPathCommand : public lldb::SBCommandPluginInterface
{
public:
    virtual bool DoExecute(lldb::SBDebugger debugger, char **command, lldb::SBCommandReturnObject &result)
    {
        if (command == NULL){
            std::cout << "The path cannot be empty" << std::endl;
            return false;
        }

        clrPath = strdup(command[0]);

        return true;
    }
};

class ManagedCommand : public lldb::SBCommandPluginInterface{
private:
    InvokeFunc* _invokeFunc;
    const char* _pluginName;
    const char* _commandName;

public:

    ManagedCommand(const char* pluginName, const char* commandName, InvokeFunc* invokeFunc){
        _invokeFunc = invokeFunc;
        _pluginName = pluginName;
        _commandName = commandName;
    }

    virtual bool DoExecute(lldb::SBDebugger debugger, char **command, lldb::SBCommandReturnObject &result)
    {
        LLDBServices* services = new LLDBServices(debugger, result);

        _invokeFunc(_pluginName, _commandName, services, command == nullptr ? "" : command[0]);

        return true;
    }
};

class LoadManagedCommand : public lldb::SBCommandPluginInterface
{
private:
    ClrInterop* _interop;

public:

    LoadManagedCommand()
    {
       _interop = new ClrInterop();
    }

    virtual bool DoExecute(lldb::SBDebugger debugger, char **command, lldb::SBCommandReturnObject &result)
    {
        auto path = command[0];

        if (!_interop->Initialized)
        {
            std::string managedAssembly;

            managedAssembly += libraryPath;
            managedAssembly += "/PluginInterop.dll";

            int status = _interop->Initialize(
                    "LoadManaged",
                    libraryPath,
                    clrPath,
                    managedAssembly.c_str());

            if (status != 0)
            {
                std::cout << "Failed to initialize the CLR" << std::endl;
                return false;
            }
        }

        char* pluginName = _interop->LoadPlugin(path);

        int exportCount = _interop->GetExportCount(pluginName);

        auto interpreter = debugger.GetCommandInterpreter();

        for (int i = 0; i < exportCount; i++){
            char* exportName = _interop->GetExportName(pluginName, i);

            auto command = new ManagedCommand(pluginName, exportName, _interop->Invoke);

            interpreter.AddCommand(exportName, command, exportName);
        }

        std::cout << "Imported " << exportCount << " functions" << std::endl;

        return true;
    }
};


static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
    if (!std::strstr(info->dlpi_name, "libloadmanaged.so")){
        return 0;
    }

    auto length = strlen(info->dlpi_name);

    char* destination = (char*) malloc(sizeof(char) * (length + 1));

    strcpy(destination, info->dlpi_name);

    libraryPath = dirname(destination);

    return 1;
}

bool LocateCoreClr(lldb::SBDebugger debugger)
{
    auto target = debugger.GetSelectedTarget();

    if (!target.IsValid())
    {
        std::cout << "Invalid debug target" << std::endl;
        return false;
    }

    int numModules = target.GetNumModules();

    for (int i = 0; i < numModules; i++)
    {
        auto module = target.GetModuleAtIndex(i);

        if (!module.IsValid())
        {
            continue;
        }

        auto fileName = module.GetFileSpec().GetFilename();

        if (strcmp(fileName, "libcoreclr.so") == 0)
        {
            clrPath = strdup(module.GetFileSpec().GetDirectory());
            return true;
        }
    }

    return false;
}

bool lldb::PluginInitialize(lldb::SBDebugger debugger)
{
    dl_iterate_phdr(callback, NULL);

    auto interpreter = debugger.GetCommandInterpreter();
    interpreter.AddCommand("SetClrPath", new SetClrPathCommand(), "Set the path to the CLR");
    interpreter.AddCommand("LoadManaged", new LoadManagedCommand(), "Load managed plugin");

    if (!LocateCoreClr(debugger))
    {
        std::cout << "Could not locate CoreCLR. Use SetClrPath to manually set the path to the CLR." << std::endl;
    }
    else
    {
        std::cout << "Found CoreCLR at \"" << clrPath << "\". Use SetClrPath to override." << std::endl;
    }

    return true;
}