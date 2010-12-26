using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegUnimpl : GIFReg
    {
        public GIFRegUnimpl(int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat) : base(addr, LowData, HighData, PackedFormat) { }

        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegUnimpl u = new GIFRegUnimpl(addr, LowData, HighData, PackedFormat);
            u.Descriptor = (GIFRegDescriptor)addr;
            return u;
        }

        public override string ToString()
        {
            return Descriptor.ToString();
        }
    }
}
