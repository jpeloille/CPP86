using Tizen.Applications;
using Uno.UI.Runtime.Skia;

namespace UnoApp_Test.Skia.Tizen
{
    class Program
    {
        static void Main(string[] args)
        {
            var host = new TizenHost(() => new UnoApp_Test.App(), args);
            host.Run();
        }
    }
}