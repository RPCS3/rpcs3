using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    abstract public class GIFReg : IGifData
    {
        public GIFRegDescriptor Descriptor;
    }

    public enum GIFRegDescriptor
    {
        PRIM = 0,
        RGBAQ = 1,
        ST = 2,
        UV = 3,
        XYZF2 = 4,
        XYZ2 = 5,
        TEX0_1 = 6,
        TEX0_2 = 7,
        CLAMP_1 = 8,
        CLAMP_2 = 9,
        FOG = 10,
        Reserved = 11,
        XYZF3 = 12,
        XYZ3 = 13,
        AD = 14,
        NOP = 15
    }
}
