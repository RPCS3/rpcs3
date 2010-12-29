using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    [Serializable]
    public class GIFRegUV : GIFReg
    {
        public double U;
        public double V;

        public GIFRegUV(byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat) : base(addr, LowData, HighData, PackedFormat) { }

        static public GIFReg Unpack(GIFTag tag, byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegUV uv = new GIFRegUV(addr, LowData, HighData, PackedFormat);
            uv.Descriptor = (GIFRegDescriptor)addr;
            if (PackedFormat)
            {
                uv.U = GetBit(LowData, 0, 14) / 16d;
                uv.V = GetBit(LowData, 32, 14) / 16d;
            }
            else
            {
                uv.U = GetBit(LowData, 0, 14) / 16d;
                uv.V = GetBit(LowData, 16, 14) / 16d;
            }
            return uv;
        }

        public override string ToString()
        {
            return Descriptor.ToString() + "@U : " + U.ToString("F4") + "@V : " + V.ToString("F4");
        }
    }
}
