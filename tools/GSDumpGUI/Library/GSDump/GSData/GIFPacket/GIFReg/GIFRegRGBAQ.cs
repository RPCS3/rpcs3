using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegRGBAQ : GIFReg
    {
        public int R;
        public int G;
        public int B;
        public int A;
        public float Q;

        public GIFRegRGBAQ(int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat) : base(addr, LowData, HighData, PackedFormat) { }

        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegRGBAQ r = new GIFRegRGBAQ(addr, LowData, HighData, PackedFormat);
            r.Descriptor = (GIFRegDescriptor)addr;
            if (PackedFormat)
            {
                r.R = (int)GetBit(LowData, 0, 8);
                r.G = (int)GetBit(LowData, 32, 8);
                r.B = (int)GetBit(HighData, 0, 8);
                r.A = (int)GetBit(HighData, 32, 8);
                r.Q = tag.Q;
            }
            else
            {
                r.R = (int)GetBit(LowData, 0, 8);
                r.G = (int)GetBit(LowData, 8, 8);
                r.B = (int)GetBit(LowData, 16, 8);
                r.A = (int)GetBit(LowData, 24, 8);
                r.Q = BitConverter.ToSingle(BitConverter.GetBytes(LowData), 4);
            }
            return r;
        }

        public override string ToString()
        {
            return Descriptor.ToString() + "@Red : " + R.ToString() + "@Green : " + G.ToString() + "@Blue : " + B.ToString() + "@Alpha : " + A.ToString();
        }
    }
}
