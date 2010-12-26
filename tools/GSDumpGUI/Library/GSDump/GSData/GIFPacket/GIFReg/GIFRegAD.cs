using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegAD : GIFReg
    {
        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            int reg = (int)GetBit(HighData, 0, 8);
            if (reg == (int)GIFRegDescriptor.AD)
                return GIFRegNOP.Unpack(tag, reg, LowData, HighData, PackedFormat);
            return GIFTag.GetUnpack(reg)(tag, reg, LowData, 0, false);
        }
    }
}
