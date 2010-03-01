using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    static public class GSDXImport
    {
        public delegate void GSReplay(IntPtr HWND, IntPtr HInstance, String File, Boolean Show);
        public delegate void GSConfigure();
        public delegate IntPtr PSEgetLibName();
    }
}
