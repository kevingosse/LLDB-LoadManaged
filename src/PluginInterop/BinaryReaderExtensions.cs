using System.IO;
using System.Text;

namespace PluginInterop
{
    internal static class BinaryReaderExtensions
    {
        public static string ReadRawString(this BinaryReader reader)
        {
            var result = new StringBuilder();

            while (true)
            {
                var c = reader.ReadChar();

                if (c == '\0')
                {
                    return result.ToString();
                }

                result.Append(c);
            }
        }
    }
}
