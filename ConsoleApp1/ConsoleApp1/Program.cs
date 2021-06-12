using System;

namespace ConsoleApp1
{
    class Program
    {
        static void Main(string[] args)
        {
            int d = 0;
            byte result;
            for (Byte c = 0; c < 255; c++)
            {
                result = c & 1;
                
                if (c.CompareTo(1) == 1) d++;
                if (c << 7 == 1) d++;
                if (c << 7 == 1) d++;
                if (c << 7 == 1) d++;
                if (c << 7 == 1) d++;
                if (c << 7 == 1) d++;
                if (c << 7 == 1) d++;

                Console.WriteLine(C.ToString());
                
            }
            Console.WriteLine("Hello World!");
        }
    }
}