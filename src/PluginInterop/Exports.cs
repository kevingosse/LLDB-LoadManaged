using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Inspect.Microsoft.Diagnostics.Runtime.Utilities;
using Mono.Cecil;

namespace PluginInterop
{
    public static class Exports
    {
        public static Export[] Get(string assemblyFileName)
        {
            // TODO: Check that the vtable is 64 bits

            var exports = GetFunctionExports(assemblyFileName);

            var assembly = AssemblyDefinition.ReadAssembly(assemblyFileName);

            var result = new List<Export>();

            foreach (var method in GetStdCallMethods(assembly))
            {
                if (ValidateParameters(method))
                {
                    var export = exports.FirstOrDefault(e => e.Token == method.MetadataToken.ToUInt32());

                    if (export == null)
                    {
                        continue;
                    }

                    result.Add(new Export(export.Name, method.DeclaringType.ToString(), method.Name));
                }
            }

            return result.ToArray();
        }

        public static int GetExportCount(string assemblyFileName)
        {
            return Get(assemblyFileName).Length;
        }

        public static string GetFirstExport(string assemblyFileName)
        {
            return Get(assemblyFileName)[0].ExportName;
        }

        public static string GetExportName(string assemblyFileName, int index)
        {
            return Get(assemblyFileName)[index].ExportName;
        }

        public static string GetTypeName(string assemblyFileName, int index)
        {
            return Get(assemblyFileName)[index].Type;
        }

        public static string GetMethodName(string assemblyFileName, int index)
        {
            return Get(assemblyFileName)[index].MethodName;
        }

        public static string[] GetExportNames(string assemblyFileName)
        {
            return Get(assemblyFileName).Select(e => e.ExportName).ToArray();
        }

        public static string GetAssemblyName(string assemblyFileName)
        {
            using (var assembly = AssemblyDefinition.ReadAssembly(assemblyFileName))
            {
                return assembly.Name.Name;
            }
        }

        public static void GetExports(string assemblyFileName, int index, char[] exportName, char[] type, char[] methodName)
        {
            var export = Get(assemblyFileName)[index];

            for (int i = 0; i < export.ExportName.Length; i++)
            {
                exportName[i] = export.ExportName[i];
            }

            for (int i = 0; i < export.Type.Length; i++)
            {
                type[i] = export.Type[i];
            }

            for (int i = 0; i < export.ExportName.Length; i++)
            {
                methodName[i] = export.ExportName[i];
            }
        }


        //public static void GetExports(string assemblyFileName, string[] exportNames, string[] types, string[] methodNames)
        //{
        //    var exports = Get(assemblyFileName);

        //    for (int i = 0; i < exports.Length; i++)
        //    {
        //        exportNames[i] = exports[i].ExportName;
        //        types[i] = exports[i].Type;
        //        methodNames[i] = exports[i].MethodName;
        //    }
        //}

        private static IReadOnlyList<FunctionExport> GetFunctionExports(string assemblyFileName)
        {
            using (var stream = File.OpenRead(assemblyFileName))
            {
                var file = new PEImage(stream);

                var exports = file.ReadFunctionExports().ToList();

                if (exports.Count == 0)
                {
                    return new List<FunctionExport>();
                }

                var buffer = new byte[file.CorHeader.VTableFixups.Size];

                file.Read(buffer, (int)file.CorHeader.VTableFixups.VirtualAddress, buffer.Length);

                var tableRva = BitConverter.ToInt32(buffer, 0);
                var numSlots = BitConverter.ToInt16(buffer, 4);
                var flags = BitConverter.ToInt16(buffer, 6);

                var reader = new BinaryReader(file.Stream);
                reader.BaseStream.Seek(file.RvaToOffset(tableRva), SeekOrigin.Begin);

                var tokens = new List<uint>();

                for (int i = 0; i < numSlots; i++)
                {
                    var token = reader.ReadUInt32();
                    reader.ReadUInt32();

                    tokens.Add(token);
                }

                for (int i = 0; i < exports.Count; i++)
                {
                    buffer = new byte[10];

                    file.Read(buffer, exports[i].Address, buffer.Length);

                    var int1 = BitConverter.ToUInt32(buffer, 2);

                    var slot = (int1 - 0x80000000 - tableRva) / 8;

                    exports[i].Token = tokens[(int)slot];
                }

                return exports;
            }
        }

        private static IEnumerable<MethodDefinition> GetStdCallMethods(AssemblyDefinition assembly)
        {
            foreach (var method in assembly.Modules.SelectMany(m => m.Types).SelectMany(t => t.Methods))
            {
                if (method.ReturnType is OptionalModifierType optionalModifierType)
                {
                    if (optionalModifierType.ModifierType.FullName == "System.Runtime.CompilerServices.CallConvStdcall")
                    {
                        yield return method;
                    }
                }
            }
        }

        private static bool ValidateParameters(MethodDefinition method)
        {
            return method.Parameters.Count == 2
                   && method.Parameters[0].ParameterType.FullName == "System.IntPtr"
                   && method.Parameters[1].ParameterType.FullName == "System.String";
        }
    }
}
