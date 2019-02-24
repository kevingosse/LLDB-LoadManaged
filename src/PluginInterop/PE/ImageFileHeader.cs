using System;
using Microsoft.Diagnostics.Runtime.Utilities.Microsoft.Diagnostics.Runtime.Interop;
using Microsoft.Diagnostics.Runtime.Utilities.Microsoft.Diagnostics.Runtime.Utilities;

namespace Microsoft.Diagnostics.Runtime.Utilities
{
    // Licensed to the .NET Foundation under one or more agreements.
    // The .NET Foundation licenses this file to you under the MIT license.
    // See the LICENSE file in the project root for more information.

    using System.Runtime.InteropServices;

    namespace Microsoft.Diagnostics.Runtime.Utilities
    {
        internal unsafe struct CV_INFO_PDB70
        {
            public const int PDB70CvSignature = 0x53445352; // RSDS in ascii

            public int CvSignature;
            public Guid Signature;
            public int Age;
            public fixed byte bytePdbFileName[1]; // Actually variable sized. 
            public string PdbFileName
            {
                get
                {
                    fixed (byte* ptr = bytePdbFileName)
                        return Marshal.PtrToStringAnsi((IntPtr)ptr);
                }
            }
        }

        internal struct IMAGE_DEBUG_DIRECTORY
        {
            public int Characteristics;
            public int TimeDateStamp;
            public short MajorVersion;
            public short MinorVersion;
            public IMAGE_DEBUG_TYPE Type;
            public int SizeOfData;
            public int AddressOfRawData;
            public int PointerToRawData;
        }

        internal enum IMAGE_DEBUG_TYPE
        {
            UNKNOWN = 0,
            COFF = 1,
            CODEVIEW = 2,
            FPO = 3,
            MISC = 4,
            BBT = 10
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        internal struct IMAGE_FILE_HEADER
        {
            public ushort Machine;
            public ushort NumberOfSections;
            public uint TimeDateStamp;
            public uint PointerToSymbolTable;
            public uint NumberOfSymbols;
            public ushort SizeOfOptionalHeader;
            public ushort Characteristics;
        }
    }

    namespace Microsoft.Diagnostics.Runtime.Interop
    {
        [StructLayout(LayoutKind.Explicit)]
        public struct IMAGE_COR20_HEADER_ENTRYPOINT
        {
            [FieldOffset(0)]
            public readonly uint Token;
            [FieldOffset(0)]
            public readonly uint RVA;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct IMAGE_COR20_HEADER
        {
            // Header versioning
            public uint cb;
            public ushort MajorRuntimeVersion;
            public ushort MinorRuntimeVersion;

            // Symbol table and startup information
            public IMAGE_DATA_DIRECTORY MetaData;
            public uint Flags;

            // The main program if it is an EXE (not used if a DLL?)
            // If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is not set, EntryPointToken represents a managed entrypoint.
            // If COMIMAGE_FLAGS_NATIVE_ENTRYPOINT is set, EntryPointRVA represents an RVA to a native entrypoint
            // (depricated for DLLs, use modules constructors intead).
            public IMAGE_COR20_HEADER_ENTRYPOINT EntryPoint;

            // This is the blob of managed resources. Fetched using code:AssemblyNative.GetResource and
            // code:PEFile.GetResource and accessible from managed code from
            // System.Assembly.GetManifestResourceStream.  The meta data has a table that maps names to offsets into
            // this blob, so logically the blob is a set of resources.
            public IMAGE_DATA_DIRECTORY Resources;
            // IL assemblies can be signed with a public-private key to validate who created it.  The signature goes
            // here if this feature is used.
            public IMAGE_DATA_DIRECTORY StrongNameSignature;

            public IMAGE_DATA_DIRECTORY CodeManagerTable; // Depricated, not used
            // Used for manged codee that has unmaanaged code inside it (or exports methods as unmanaged entry points)
            public IMAGE_DATA_DIRECTORY VTableFixups;
            public IMAGE_DATA_DIRECTORY ExportAddressTableJumps;

            // null for ordinary IL images.  NGEN images it points at a code:CORCOMPILE_HEADER structure
            public IMAGE_DATA_DIRECTORY ManagedNativeHeader;
        }

        public enum IMAGE_FILE_MACHINE : uint
        {
            UNKNOWN = 0,
            I386 = 0x014c, // Intel 386.
            R3000 = 0x0162, // MIPS little-endian, 0x160 big-endian
            R4000 = 0x0166, // MIPS little-endian
            R10000 = 0x0168, // MIPS little-endian
            WCEMIPSV2 = 0x0169, // MIPS little-endian WCE v2
            ALPHA = 0x0184, // Alpha_AXP
            SH3 = 0x01a2, // SH3 little-endian
            SH3DSP = 0x01a3,
            SH3E = 0x01a4, // SH3E little-endian
            SH4 = 0x01a6, // SH4 little-endian
            SH5 = 0x01a8, // SH5
            ARM = 0x01c0, // ARM Little-Endian
            THUMB = 0x01c2,
            THUMB2 = 0x1c4,
            AM33 = 0x01d3,
            POWERPC = 0x01F0, // IBM PowerPC Little-Endian
            POWERPCFP = 0x01f1,
            IA64 = 0x0200, // Intel 64
            MIPS16 = 0x0266, // MIPS
            ALPHA64 = 0x0284, // ALPHA64
            MIPSFPU = 0x0366, // MIPS
            MIPSFPU16 = 0x0466, // MIPS
            AXP64 = 0x0284,
            TRICORE = 0x0520, // Infineon
            CEF = 0x0CEF,
            EBC = 0x0EBC, // EFI Byte Code
            AMD64 = 0x8664, // AMD64 (K8)
            M32R = 0x9041, // M32R little-endian
            CEE = 0xC0EE
        }
    }

    /// <summary>
    /// A wrapper over IMAGE_FILE_HEADER.  See https://msdn.microsoft.com/en-us/library/windows/desktop/ms680313(v=vs.85).aspx.
    /// </summary>
    public class ImageFileHeader
    {
        private IMAGE_FILE_HEADER _header;

        internal ImageFileHeader(IMAGE_FILE_HEADER header)
        {
            _header = header;
        }

        /// <summary>
        /// The architecture type of the computer. An image file can only be run on the specified computer or a system that emulates the specified computer. 
        /// </summary>
        public IMAGE_FILE_MACHINE Machine => (IMAGE_FILE_MACHINE)_header.Machine;

        /// <summary>
        /// The number of sections. This indicates the size of the section table, which immediately follows the headers. Note that the Windows loader limits the number of sections to 96.
        /// </summary>
        public ushort NumberOfSections => _header.NumberOfSections;

        /// <summary>
        /// The offset of the symbol table, in bytes, or zero if no COFF symbol table exists.
        /// </summary>
        public uint PointerToSymbolTable => _header.PointerToSymbolTable;

        /// <summary>
        /// The number of symbols in the symbol table.
        /// </summary>
        public uint NumberOfSymbols => _header.NumberOfSymbols;

        /// <summary>
        /// The low 32 bits of the time stamp of the image. This represents the date and time the image was created by the linker. The value is represented in the number of seconds elapsed since midnight (00:00:00), January 1, 1970, Universal Coordinated Time, according to the system clock.
        /// </summary>
        public uint TimeDateStamp => _header.TimeDateStamp;

        /// <summary>
        /// The size of the optional header, in bytes. This value should be 0 for object files.
        /// </summary>
        public ushort SizeOfOptionalHeader => _header.SizeOfOptionalHeader;

        /// <summary>
        /// The characteristics of the image
        /// </summary>
        public IMAGE_FILE Characteristics => (IMAGE_FILE)_header.Characteristics;
    }
}
