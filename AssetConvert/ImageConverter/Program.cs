using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ImageConverter
{
    class Program
    {
        static void Main(string[] args)
        {
            foreach (var fileName in args)
            {
                Console.WriteLine(fileName);

                var name = "IMG_" + Path.GetFileNameWithoutExtension(fileName);

                var headerFileName = name + ".cpp";
                Directory.SetCurrentDirectory(Path.GetDirectoryName(fileName) ?? Path.GetPathRoot(fileName));

                var img = new Bitmap(fileName);

                using (var f = new System.IO.StreamWriter(headerFileName))
                {
                    f.WriteLine("#include \"images.h\"");
                    f.WriteLine("#include <avr/pgmspace.h>");
                    f.WriteLine();

                    f.WriteLine($"const unsigned char {name}[] PROGMEM = ");
                    f.WriteLine("{");

                    var buffer = new byte[img.Width * img.Height];
                    int byteIdx = 0;
                    for (int y = 0; y < img.Height; y++)
                    {
                        for (int x = 0; x < img.Width; x++)
                        {
                            Color pix = img.GetPixel(x, y);
                            byte pixelValue = (byte)(pix.R > 128 ? 1 : 0);
                            if (byteIdx == 0 || (buffer[byteIdx - 1] & 1) != pixelValue || (buffer[byteIdx - 1] >> 1) >= 127)
                            {
                                buffer[byteIdx] = pixelValue;
                                byteIdx++;
                            }
                            else
                            {
                                buffer[byteIdx - 1] += 0b10;
                            }
                        }
                    }

                    for (int i = 0; i < byteIdx; i++)
                    {
                        if (i % 30 == 0)
                        {
                            if (i > 0)
                                f.WriteLine();
                            f.Write("\t");                            
                        }
                        f.Write("0x");
                        f.Write(buffer[i].ToString("X2"));
                        f.Write(", ");
                    }
                    f.WriteLine();
                    f.WriteLine("};");
                }
            }
        }
    }
}
