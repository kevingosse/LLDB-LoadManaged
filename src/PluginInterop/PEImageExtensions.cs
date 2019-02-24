using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using Inspect.Microsoft.Diagnostics.Runtime.Utilities;
using Inspect.PE;

namespace PluginInterop
{
    internal static class PEImageExtensions
    {
        public static IEnumerable<FunctionExport> ReadFunctionExports(this PEImage file)
        {
            var exports = file.OptionalHeader.ExportDirectory;

            if (exports.Size == 0)
            {
                yield break;
            }

            var buffer = new byte[exports.Size];

            file.Read(buffer, exports.VirtualAddress, exports.Size);

            var exp = MemoryMarshal.Read<IMAGE_EXPORT_DIRECTORY>(buffer);

            var reader = new BinaryReader(file.Stream);

            var nameAddresses = new List<int>();
            var names = new List<string>();

            reader.BaseStream.Seek(file.RvaToOffset((int)exp.AddressOfNames), SeekOrigin.Begin);

            for (int i = 0; i < exp.NumberOfNames; i++)
            {
                nameAddresses.Add(reader.ReadInt32());
            }

            foreach (var address in nameAddresses)
            {
                reader.BaseStream.Seek(file.RvaToOffset(address), SeekOrigin.Begin);

                names.Add(reader.ReadRawString());
            }

            var ordinals = new List<int>();

            reader.BaseStream.Seek(file.RvaToOffset((int)exp.AddressOfNameOrdinals), SeekOrigin.Begin);

            for (int i = 0; i < exp.NumberOfNames; i++)
            {
                ordinals.Add(reader.ReadInt16());
            }

            var addresses = new List<int>();

            reader.BaseStream.Seek(file.RvaToOffset((int)exp.AddressOfFunctions), SeekOrigin.Begin);

            for (int i = 0; i < exp.NumberOfFunctions; i++)
            {
                var address = reader.ReadInt32();

                int indexOfOrdinal = -1;

                for (int j = 0; j < ordinals.Count; j++)
                {
                    if (ordinals[j] == i)
                    {
                        indexOfOrdinal = j;
                    }
                }

                yield return new FunctionExport(names[indexOfOrdinal], address);
            }
        }
    }
}
