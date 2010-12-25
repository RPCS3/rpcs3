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

        static public  GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            GIFRegRGBAQ r = new GIFRegRGBAQ();
            r.Descriptor = GIFRegDescriptor.RGBAQ;
            r.R = (int)(LowData & 0xFF);
            r.G = (int)((LowData & 0xFF00000000) >> 32);
            r.B = (int)((HighData & 0xFF));
            r.A = (int)((HighData & 0xFF00000000) >> 32);
            return r;
        }
    }
}
