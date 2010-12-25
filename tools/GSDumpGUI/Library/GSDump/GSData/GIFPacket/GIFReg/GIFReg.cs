using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    abstract public class GIFReg : IGifData
    {
        public GIFRegDescriptor Descriptor;

        static public UInt64 GetBit(UInt64 value, byte lower, byte count)
        {
            return (value >> lower) & (ulong)((Math.Pow(2, count)) - 1);
        }
    }

    public enum GIFRegDescriptor
    {
        PRIM = 0x00,
        RGBAQ = 0x01,
        ST = 0x02,
        UV = 0x03,
        XYZF2 = 0x04,
        XYZ2 = 0x05,
        TEX0_1 = 0x06,
        TEX0_2 = 0x07,
        CLAMP_1 = 0x08,
        CLAMP_2 = 0x09,
        FOG = 0x0a,
        Reserved = 0x0b,
        XYZF3 = 0x0c,
        XYZ3 = 0x0d,
        AD = 0x0e,
        NOP = 0x0f
    }

}
