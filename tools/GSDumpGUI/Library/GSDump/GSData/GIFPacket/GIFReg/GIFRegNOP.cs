using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    [Serializable]
    public class GIFRegNOP : GIFReg
    {
        public byte addr;

        public GIFRegNOP(byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat) : base(addr, LowData, HighData, PackedFormat) { }

        static public GIFReg Unpack(GIFTag tag, byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegNOP nop = new GIFRegNOP(addr, LowData, HighData, PackedFormat);
            nop.Descriptor = GIFRegDescriptor.NOP;

            return nop;
        }

        public override string ToString()
        {
            return Descriptor.ToString() + " (0x" + addr.ToString("X2") + ")";
        }
    }
}
