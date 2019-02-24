using System.Collections.Generic;
using System.Reflection;

namespace PluginInterop
{
    public class Plugin
    {
        public IDictionary<string, Export> Exports { get; set; }
        public Assembly Assembly { get; set; }
    }
}