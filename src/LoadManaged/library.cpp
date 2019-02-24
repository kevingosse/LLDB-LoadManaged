#include "library.h"

#include <iostream>
#include <cstdio>
#include "coreruncommon.h"
#include "services.h"
#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBCommandInterpreter.h"
#include "lldb/API/SBCommandReturnObject.h"
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

    }
};

class LoadManagedCommand : public lldb::SBCommandPluginInterface
{
public:
    virtual bool DoExecute(lldb::SBDebugger debugger, char **command, lldb::SBCommandReturnObject &result)
    {
        auto path = command[0];

        std::string managedAssembly;

        managedAssembly += libraryPath;
        managedAssembly += "/PluginInterop.dll";

        ClrInterop* interop = new ClrInterop();

        interop->Initialize(
                "LoadManaged",
                libraryPath,
                clrPath,
                managedAssembly.c_str());

        auto loadPlugin = (LoadPluginFunc*)interop->CreateDelegate(
                "PluginInterop",
                "PluginInterop.PluginLoader",
                "LoadPlugin");

        char* pluginName = loadPlugin(path);

        auto getExportCount = (GetExportCountFunc*)interop->CreateDelegate(
                "PluginInterop",
                "PluginInterop.PluginLoader",
                "GetExportCount");

        auto getExportName = (GetExportNameFunc*)interop->CreateDelegate(
                "PluginInterop",
                "PluginInterop.PluginLoader",
                "GetExportName");

        int exportCount = getExportCount(pluginName);

        auto interpreter = debugger.GetCommandInterpreter();

        for (int i = 0; i < exportCount; i++){
            char* exportName = getExportName(pluginName, i);

            auto invokeFunc = (InvokeFunc*) interop->CreateDelegate(
                    "PluginInterop",
                    "PluginInterop.PluginLoader",
                    "Invoke");

            auto command = new ManagedCommand(pluginName, exportName, invokeFunc);

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

bool lldb::PluginInitialize(lldb::SBDebugger debugger)
{
    dl_iterate_phdr(callback, NULL);

    auto interpreter = debugger.GetCommandInterpreter();
    interpreter.AddCommand("SetClrPath", new SetClrPathCommand(), "Set the path to the CLR");
    interpreter.AddCommand("LoadManaged", new LoadManagedCommand(), "Load managed plugin");
    return true;
}