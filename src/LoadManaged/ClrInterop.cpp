#include <iostream>

#include "coreruncommon.h"
#include <string>
#include <set>
#include <limits.h>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include "coreclrhost.h"

// Name of the environment variable controlling server GC.
// If set to 1, server GC is enabled on startup. If 0, server GC is
// disabled. Server GC is off by default.
static const char* serverGcVar = "COMPlus_gcServer";

// Name of environment variable to control "System.Globalization.Invariant"
// Set to 1 for Globalization Invariant mode to be true. Default is false.
static const char* globalizationInvariantVar = "CORECLR_GLOBAL_INVARIANT";

#ifndef SUCCEEDED
#define SUCCEEDED(Status) ((Status) >= 0)
#endif // !SUCCEEDED

class ClrInterop
{
    coreclr_initialize_ptr initializeCoreCLR;
    coreclr_create_delegate_ptr createDelegate;
    coreclr_shutdown_2_ptr shutdownCoreCLR;

    void* hostHandle;
    unsigned int domainId;

    bool GetDirectory(const char* absolutePath, std::string& directory)
    {
        directory.assign(absolutePath);
        size_t lastSlash = directory.rfind('/');
        if (lastSlash != std::string::npos)
        {
            directory.erase(lastSlash);
            return true;
        }

        return false;
    }

    void AddFilesFromDirectoryToTpaList(const char* directory, std::string& tpaList)
    {
        const char * const tpaExtensions[] = {
                ".ni.dll",      // Probe for .ni.dll first so that it's preferred if ni and il coexist in the same dir
                ".dll",
                ".ni.exe",
                ".exe",
        };

        DIR* dir = opendir(directory);
        if (dir == nullptr)
        {
            return;
        }

        std::set<std::string> addedAssemblies;

        // Walk the directory for each extension separately so that we first get files with .ni.dll extension,
        // then files with .dll extension, etc.
        for (int extIndex = 0; extIndex < sizeof(tpaExtensions) / sizeof(tpaExtensions[0]); extIndex++)
        {
            const char* ext = tpaExtensions[extIndex];
            int extLength = strlen(ext);

            struct dirent* entry;

            // For all entries in the directory
            while ((entry = readdir(dir)) != nullptr)
            {
                // We are interested in files only
                switch (entry->d_type)
                {
                    case DT_REG:
                        break;

                        // Handle symlinks and file systems that do not support d_type
                    case DT_LNK:
                    case DT_UNKNOWN:
                    {
                        std::string fullFilename;

                        fullFilename.append(directory);
                        fullFilename.append("/");
                        fullFilename.append(entry->d_name);

                        struct stat sb;
                        if (stat(fullFilename.c_str(), &sb) == -1)
                        {
                            continue;
                        }

                        if (!S_ISREG(sb.st_mode))
                        {
                            continue;
                        }
                    }
                        break;

                    default:
                        continue;
                }

                std::string filename(entry->d_name);

                // Check if the extension matches the one we are looking for
                int extPos = filename.length() - extLength;
                if ((extPos <= 0) || (filename.compare(extPos, extLength, ext) != 0))
                {
                    continue;
                }

                std::string filenameWithoutExt(filename.substr(0, extPos));

                // Make sure if we have an assembly with multiple extensions present,
                // we insert only one version of it.
                if (addedAssemblies.find(filenameWithoutExt) == addedAssemblies.end())
                {
                    addedAssemblies.insert(filenameWithoutExt);

                    tpaList.append(directory);
                    tpaList.append("/");
                    tpaList.append(filename);
                    tpaList.append(":");
                }
            }

            // Rewind the directory stream to be able to iterate over it for the next extension
            rewinddir(dir);
        }

        closedir(dir);
    }

    const char* GetEnvValueBoolean(const char* envVariable)
    {
        const char* envValue = std::getenv(envVariable);
        if (envValue == nullptr)
        {
            envValue = "0";
        }
        // CoreCLR expects strings "true" and "false" instead of "1" and "0".
        return (std::strcmp(envValue, "1") == 0 || strcasecmp(envValue, "true") == 0) ? "true" : "false";
    }

    bool InitializeDelegates()
    {
        GetExportCount = (GetExportCountFunc*)CreateDelegate(
                "PluginInterop",
                "PluginInterop.PluginLoader",
                "GetExportCount");

        if (GetExportCount == nullptr)
        {
            std::cout << "Could not find function GetExportCount in PluginInterop.dll" << std::endl;
            return false;
        }

        GetExportName = (GetExportNameFunc*)CreateDelegate(
                "PluginInterop",
                "PluginInterop.PluginLoader",
                "GetExportName");

        if (GetExportName == nullptr)
        {
            std::cout << "Could not find function GetExportName in PluginInterop.dll" << std::endl;
            return false;
        }

        LoadPlugin = (LoadPluginFunc*)CreateDelegate(
                "PluginInterop",
                "PluginInterop.PluginLoader",
                "LoadPlugin");

        if (LoadPlugin == nullptr)
        {
            std::cout << "Could not find function LoadPlugin in PluginInterop.dll" << std::endl;
            return false;
        }

        Invoke = (InvokeFunc*)CreateDelegate(
                "PluginInterop",
                "PluginInterop.PluginLoader",
                "Invoke");

        if (Invoke == nullptr)
        {
            std::cout << "Could not find function Invoke in PluginInterop.dll" << std::endl;
            return false;
        }

        return true;
    }

public:
    bool Initialized;

    GetExportCountFunc* GetExportCount;
    GetExportNameFunc* GetExportName;
    LoadPluginFunc* LoadPlugin;
    InvokeFunc* Invoke;

    int Initialize(
            const char* name,
            const char* currentExeAbsolutePath,
            const char* clrFilesAbsolutePath,
            const char* managedAssemblyAbsolutePath)
    {
        // Indicates failure
        int exitCode = -1;

        std::string coreClrDllPath(clrFilesAbsolutePath);
        coreClrDllPath.append("/");
        coreClrDllPath.append(coreClrDll);

        if (coreClrDllPath.length() >= PATH_MAX)
        {
            fprintf(stderr, "Absolute path to libcoreclr.so too long\n");
            return -1;
        }

        // Get just the path component of the managed assembly path
        std::string appPath;
        GetDirectory(managedAssemblyAbsolutePath, appPath);

        std::string tpaList;
        if (strlen(managedAssemblyAbsolutePath) > 0)
        {
            // Target assembly should be added to the tpa list. Otherwise corerun.exe
            // may find wrong assembly to execute.
            // Details can be found at https://github.com/dotnet/coreclr/issues/5631
            tpaList = managedAssemblyAbsolutePath;
            tpaList.append(":");
        }

        // Construct native search directory paths
        std::string nativeDllSearchDirs(appPath);
        char *coreLibraries = getenv("CORE_LIBRARIES");
        if (coreLibraries)
        {
            nativeDllSearchDirs.append(":");
            nativeDllSearchDirs.append(coreLibraries);
            if (std::strcmp(coreLibraries, clrFilesAbsolutePath) != 0)
            {
                AddFilesFromDirectoryToTpaList(coreLibraries, tpaList);
            }
        }

        nativeDllSearchDirs.append(":");
        nativeDllSearchDirs.append(clrFilesAbsolutePath);

        AddFilesFromDirectoryToTpaList(clrFilesAbsolutePath, tpaList);

        void* coreclrLib = dlopen(coreClrDllPath.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (coreclrLib != nullptr)
        {
            initializeCoreCLR = (coreclr_initialize_ptr)dlsym(coreclrLib, "coreclr_initialize");
            createDelegate = (coreclr_create_delegate_ptr)dlsym(coreclrLib, "coreclr_create_delegate");
            shutdownCoreCLR = (coreclr_shutdown_2_ptr)dlsym(coreclrLib, "coreclr_shutdown_2");

            if (initializeCoreCLR == nullptr)
            {
                fprintf(stderr, "Function coreclr_initialize not found in the libcoreclr.so\n");
            }
            else if (createDelegate == nullptr)
            {
                fprintf(stderr, "Function coreclr_create_delegate not found in the libcoreclr.so\n");
            }
            else if (shutdownCoreCLR == nullptr)
            {
                fprintf(stderr, "Function coreclr_shutdown_2 not found in the libcoreclr.so\n");
            }
            else
            {
                // Check whether we are enabling server GC (off by default)
                const char* useServerGc = GetEnvValueBoolean(serverGcVar);

                // Check Globalization Invariant mode (false by default)
                const char* globalizationInvariant = GetEnvValueBoolean(globalizationInvariantVar);

                // Allowed property names:
                // APPBASE
                // - The base path of the application from which the exe and other assemblies will be loaded
                //
                // TRUSTED_PLATFORM_ASSEMBLIES
                // - The list of complete paths to each of the fully trusted assemblies
                //
                // APP_PATHS
                // - The list of paths which will be probed by the assembly loader
                //
                // APP_NI_PATHS
                // - The list of additional paths that the assembly loader will probe for ngen images
                //
                // NATIVE_DLL_SEARCH_DIRECTORIES
                // - The list of paths that will be probed for native DLLs called by PInvoke
                //
                const char *propertyKeys[] = {
                        "TRUSTED_PLATFORM_ASSEMBLIES",
                        "APP_PATHS",
                        "APP_NI_PATHS",
                        "NATIVE_DLL_SEARCH_DIRECTORIES",
                        "System.GC.Server",
                        "System.Globalization.Invariant",
                };
                const char *propertyValues[] = {
                        // TRUSTED_PLATFORM_ASSEMBLIES
                        tpaList.c_str(),
                        // APP_PATHS
                        appPath.c_str(),
                        // APP_NI_PATHS
                        appPath.c_str(),
                        // NATIVE_DLL_SEARCH_DIRECTORIES
                        nativeDllSearchDirs.c_str(),
                        // System.GC.Server
                        useServerGc,
                        // System.Globalization.Invariant
                        globalizationInvariant,
                };

                int st = initializeCoreCLR(
                        currentExeAbsolutePath,
                        name,
                        sizeof(propertyKeys) / sizeof(propertyKeys[0]),
                        propertyKeys,
                        propertyValues,
                        &hostHandle,
                        &domainId);

                if (!SUCCEEDED(st))
                {
                    fprintf(stderr, "coreclr_initialize failed - status: 0x%08x\n", st);
                    return -1;
                }
                else
                {
                    Initialized = true;

                    if (!InitializeDelegates())
                    {
                        std::cout << "An error occured while calling PluginInterop.dll. "
                        << "Make sure the right version of the file is located in " << currentExeAbsolutePath << std::endl;
                        return -1;
                    }

                    return 0;
                }
            }

            if (dlclose(coreclrLib) != 0)
            {
                fprintf(stderr, "Warning - dlclose failed\n");
            }
        }
        else
        {
            const char* error = dlerror();
            fprintf(stderr, "dlopen failed to open the libcoreclr.so with error %s\n", error);
        }

        return -1;
    }

    void* CreateDelegate(
            const char* assemblyName,
            const char* typeName,
            const char* methodName)
    {

        void* pfnDelegate = NULL;

        int st = createDelegate(
                hostHandle,
                domainId,
                assemblyName,
                typeName,
                methodName,
                &pfnDelegate);

        return pfnDelegate;
    }
};