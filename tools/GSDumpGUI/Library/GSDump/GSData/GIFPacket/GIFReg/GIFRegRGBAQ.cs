using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    [Serializable]
    public class GIFRegRGBAQ : GIFReg
    {
        public byte R;
        public byte G;
        public byte B;
        public byte A;
        public float Q;

        public GIFRegRGBAQ(byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat) : base(addr, LowData, HighData, PackedFormat) { }

        static public GIFReg Unpack(GIFTag tag, byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegRGBAQ r = new GIFRegRGBAQ(addr, LowData, HighData, PackedFormat);
            r.Descriptor = (GIFRegDescriptor)addr;
            if (PackedFormat)
            {
                r.R = (byte)GetBit(LowData, 0, 8);
                r.G = (byte)GetBit(LowData, 32, 8);
                r.B = (byte)GetBit(HighData, 0, 8);
                r.A = (byte)GetBit(HighData, 32, 8);
                r.Q = tag.Q;
            }
            else
            {
                r.R = (byte)GetBit(LowData, 0, 8);
                r.G = (byte)GetBit(LowData, 8, 8);
                r.B = (byte)GetBit(LowData, 16, 8);
                r.A = (byte)GetBit(LowData, 24, 8);
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
