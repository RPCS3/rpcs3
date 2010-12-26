using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegUV : GIFReg
    {
        public double U;
        public double V;

        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegUV uv = new GIFRegUV();
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
