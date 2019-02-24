using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace PluginInterop
{
    public class PluginLoader
    {
        private static readonly Dictionary<string, Plugin> Plugins = new Dictionary<string, Plugin>();

        public static string LoadPlugin(string path)
        {
            var loadContext = new PluginLoadContext(path);

            var assembly = loadContext.LoadFromAssemblyPath(path);

            var plugin = new Plugin();

            plugin.Exports = Exports.Get(path).ToDictionary(e => e.ExportName, e => e);
            plugin.Assembly = assembly;

            var name = assembly.GetName().Name;

            Plugins.Add(name, plugin);

            return name;
        }

        public static int GetExportCount(string pluginName)
        {
            return Plugins[pluginName].Exports.Count;
        }

        public static string GetExportName(string pluginName, int index)
        {
            return Plugins[pluginName].Exports.Values.ElementAt(index).ExportName;
        }

        public static void Invoke(string pluginName, string exportName, IntPtr debugClient, [MarshalAs(UnmanagedType.LPStr)] string args)
        {
            var plugin = Plugins[pluginName];
            var export = Plugins[pluginName].Exports[exportName];

            var type = plugin.Assembly.GetType(export.Type);

            if (type == null)
            {
                Console.WriteLine("Could not locate type {0} in assembly {1}", export.Type, plugin.Assembly);
                return;
            }

            var method = type.GetMethod(export.MethodName);

            if (method == null)
            {
                Console.WriteLine("Could not locate method {0} in assembly {1}", method, plugin.Assembly);
                return;
            }

            method.Invoke(null, new object[] { debugClient, args });
        }
    }
}
