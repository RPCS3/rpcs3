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

                ulong pt = ((LowData & 0xFFFFFFFF));
                void* ptt = &pt;
                st.S = *(float*)ptt;

                pt = ((LowData & 0xFFFFFFFF00000000) >> 32);
                ptt = &pt;
                st.T = *(float*)ptt;

                pt = ((HighData & 0xFFFFFFFF));
                ptt = &pt;
                st.Q = *(float*)ptt;
                return st;
            }
        }
    }
}
