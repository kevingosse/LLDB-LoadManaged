namespace PluginInterop
{
    public class FunctionExport
    {
        public string Name;
        public int Address;
        public uint Token;

        public FunctionExport(string name, int address)
        {
            Name = name;
            Address = address;
        }
    }
}
