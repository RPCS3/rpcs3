using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegFOG : GIFReg
    {
        public double F;

        public GIFRegFOG(byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat) : base(addr, LowData, HighData, PackedFormat) { }

        static public GIFReg Unpack(GIFTag tag, byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegFOG u = new GIFRegFOG(addr, LowData, HighData, PackedFormat);
            u.Descriptor = (GIFRegDescriptor)addr;
            if (PackedFormat)
                u.F = (UInt16)(GetBit(HighData, 36, 8));
            else
                u.F = GetBit(LowData, 56, 8);
            return u;
        }

        public override string ToString()
        {
            return Descriptor.ToString() + "@F : " + F.ToString();
        }
    }
}
