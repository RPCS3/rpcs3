using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegNOP : GIFReg
    {
        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegNOP nop = new GIFRegNOP();
            nop.Descriptor = GIFRegDescriptor.NOP;
            return nop;
        }
    }
}
