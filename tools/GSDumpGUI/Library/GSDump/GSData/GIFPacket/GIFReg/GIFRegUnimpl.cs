using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegUnimpl : GIFReg
    {
        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegUnimpl u = new GIFRegUnimpl();
            u.Descriptor = (GIFRegDescriptor)addr;
            return u;
        }
    }
}
