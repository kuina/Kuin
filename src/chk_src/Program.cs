using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace chk_src
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.WriteLine("Drop me the root folder.");
                Console.ReadKey();
                return;
            }
            int line_num = 0;
            string[] exts = new[] { ".c", ".h", ".kn", ".cpp", ".fx" };
            string[] files = System.IO.Directory.GetFiles(args[0], "*", System.IO.SearchOption.AllDirectories).Where(file => exts.Any(file.ToLower().EndsWith)).ToArray();
            foreach (string file in files)
            {
                string ext = file.ToLower().Substring(file.LastIndexOf('.') + 1);
                Encoding encoding = ext == "kn" ? (new System.Text.UTF8Encoding(false)) : Encoding.ASCII;
                Console.WriteLine("[" + ext + "] " + file);
                System.IO.StreamReader read_ptr = new System.IO.StreamReader(file);
                System.IO.StreamWriter write_ptr = new System.IO.StreamWriter(file + "_tmp", false, encoding);
                while (!read_ptr.EndOfStream)
                {
                    string str = read_ptr.ReadLine();
                    str = str.TrimEnd();
                    string str2 = "";
                    bool tab = false;
                    int len = str.Length;
                    for (int i = 0; i < len; i++)
                    {
                        if (str[i] != '\t')
                            tab = true;
                        else if (tab)
                            continue;
                        if (i + 3 < len && str[i] == '/' && str[i + 1] == '/' && str[i + 2] == '<' && str[i + 3] == '-')
                        {
                            str2 += "// TODO:";
                            break;
                        }
                        str2 += str[i].ToString();
                    }
                    str2 = str2.TrimEnd();
                    write_ptr.WriteLine(str2);
                    line_num++;
                }
                write_ptr.Close();
                read_ptr.Close();
                System.IO.File.Copy(file + "_tmp", file, true);
                Console.WriteLine("OK: " + line_num);
            }
            Console.WriteLine("Success.");
            Console.ReadKey();
        }
    }
}
