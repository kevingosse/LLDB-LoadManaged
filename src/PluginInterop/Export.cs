using System.Runtime.InteropServices;

namespace PluginInterop
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Export
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string ExportName;
        [MarshalAs(UnmanagedType.LPStr)]
        public string Type;
        [MarshalAs(UnmanagedType.LPStr)]
        public string MethodName;

        public Export(string exportName, string type, string methodName)
        {
            ExportName = exportName;
            Type = type;
            MethodName = methodName;
        }
    }
}