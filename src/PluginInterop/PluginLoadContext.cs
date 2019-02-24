using System;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;

namespace PluginInterop
{
    public class PluginLoadContext : AssemblyLoadContext
    {
        private readonly string _basePath;

        public PluginLoadContext(string path)
        {
            _basePath = Path.GetDirectoryName(path);
        }

        protected override Assembly Load(AssemblyName assemblyName)
        {
            try
            {
                var assembly = LoadFromAssemblyPath(Path.Combine(_basePath, assemblyName.Name + ".dll"));

                return assembly ?? LoadFromAssemblyName(assemblyName);
            }
            catch (Exception)
            {
                return AssemblyLoadContext.Default.LoadFromAssemblyName(assemblyName);
            }
        }
    }
}