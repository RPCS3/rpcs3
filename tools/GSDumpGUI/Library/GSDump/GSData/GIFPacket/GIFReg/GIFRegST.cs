using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegST : GIFReg
    {
        public float S;
        public float T;
        public float Q;

        public bool isSTQ;

        public GIFRegST(byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat) : base(addr, LowData, HighData, PackedFormat) { }

        static public GIFReg Unpack(GIFTag tag, byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegST st = new GIFRegST(addr, LowData, HighData, PackedFormat);
            st.Descriptor = (GIFRegDescriptor)addr;

            st.S = BitConverter.ToSingle(BitConverter.GetBytes(LowData), 0);
            st.T = BitConverter.ToSingle(BitConverter.GetBytes(LowData), 4);
            if (PackedFormat)
            {
                st.Q = BitConverter.ToSingle(BitConverter.GetBytes(HighData), 0);
                tag.Q = st.Q;
                st.isSTQ = true;
            }
            else
                st.isSTQ = false;

            return st;
        }

        public override string ToString()
        {
            return Descriptor.ToString() + "@S : " + S.ToString("F8") + "@T : " + T.ToString("F8") + (isSTQ ? "@Q : " + Q.ToString("F8") : "");
        }
    }
}
