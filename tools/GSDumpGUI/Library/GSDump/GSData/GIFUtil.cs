using System;

namespace GSDumpGUI
{
    public class GIFUtil
    {
        public static UInt64 GetBit(UInt64 value, int lower, int count)
        {
            return (value >> lower) & (ulong)((1ul << count) - 1);
        }
    }
}