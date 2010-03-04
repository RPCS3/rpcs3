using System;
using System.Collections.Generic;
using System.Text;
using System.Security;
using System.Runtime.InteropServices;

namespace GSDumpGUI
{
    static public class NativeMethods
    {
        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("kernel32")]
        public extern static IntPtr LoadLibrary(string lpLibFileName);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("kernel32")]
        public extern static bool FreeLibrary(IntPtr hLibModule);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("kernel32", CharSet = CharSet.Ansi)]
        public extern static IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("kernel32", CharSet = CharSet.Ansi)]
        public extern static int SetErrorMode(int Value);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("kernel32", CharSet = CharSet.Ansi)]
        public extern static int GetLastError();

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("user32", CharSet = CharSet.Ansi)]
        public extern static short GetAsyncKeyState(int key);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("user32", CharSet = CharSet.Ansi)]
        public extern static int SetClassLong(IntPtr HWND, int index, long newlong);
    }
}
