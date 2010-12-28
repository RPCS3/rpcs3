using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public static class GIFRegAD
    {
        static public GIFReg Unpack(GIFTag tag, byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            byte reg = (byte)GIFReg.GetBit(HighData, 0, 8);
            if (reg == (byte)GIFRegDescriptor.AD)
                return GIFRegNOP.Unpack(tag, reg, LowData, HighData, PackedFormat);
            return GIFTag.GetUnpack(reg)(tag, reg, LowData, HighData, false);
        }
    }
}
