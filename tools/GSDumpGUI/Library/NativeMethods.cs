using System;
using System.Collections.Generic;
using System.Text;
using System.Security;
using System.Runtime.InteropServices;
using System.Drawing;

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

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("user32", CharSet = CharSet.Ansi)]
        public extern static bool IsWindowVisible(IntPtr HWND);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool PeekMessage(out NativeMessage message, IntPtr hwnd, uint messageFilterMin, uint messageFilterMax, uint flags);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool TranslateMessage(ref NativeMessage message);

        [SuppressUnmanagedCodeSecurityAttribute]
        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DispatchMessage(ref NativeMessage message);
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeMessage
    {
        public IntPtr hWnd;
        public uint msg;
        public IntPtr wParam;
        public IntPtr lParam;
        public uint time;
        public Point p;
    }

}
