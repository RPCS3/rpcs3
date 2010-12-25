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

        static public GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            unsafe
            {
                GIFRegST st = new GIFRegST();
                st.Descriptor = GIFRegDescriptor.ST;

                ulong pt = GIFReg.GetBit(LowData, 0, 32);
                void* ptt = &pt;
                st.S = *(float*)ptt;

                pt = GIFReg.GetBit(LowData, 32, 32);
                ptt = &pt;
                st.T = *(float*)ptt;

                pt = GIFReg.GetBit(HighData, 0, 32);
                ptt = &pt;
                st.Q = *(float*)ptt;
                return st;
            }
        }
    }
}
