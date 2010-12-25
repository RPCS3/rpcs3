using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegPrim : GIFReg
    {
        public GS_PRIM PrimitiveType;
        public GSIIP IIP;
        public bool TME;
        public bool FGE;
        public bool ABE;
        public bool AA1;
        public GSFST FST;
        public GSCTXT CTXT;
        public GSFIX FIX;

        static public GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            GIFRegPrim pr = new GIFRegPrim();
            pr.Descriptor = GIFRegDescriptor.PRIM;
            pr.PrimitiveType = (GS_PRIM)(LowData & 0x7);
            pr.IIP = (GSIIP)((LowData & 0x8) >> 3);
            pr.TME = Convert.ToBoolean(((LowData & 0x10) >> 4));
            pr.FGE = Convert.ToBoolean(((LowData & 0x20) >> 5));
            pr.ABE = Convert.ToBoolean(((LowData & 0x40) >> 6));
            pr.AA1 = Convert.ToBoolean(((LowData & 0x80) >> 7));
            pr.FST = (GSFST)((LowData & 0x100) >> 8);
            pr.CTXT = (GSCTXT)((LowData & 0x200) >> 9);
            pr.FIX = (GSFIX)((LowData & 0x400) >> 10);
            return pr;
        }
    }

    public enum GSIIP
    {
        FlatShading=0,
        Gouraud=1
    }

    public enum GSFST
    {
        STQValue=0,
        UVValue=1
    }

    public enum GSCTXT
    {
        Context1 =0,
        Context2 =1
    }

    public enum GSFIX
    {
        Unfixed =0,
        Fixed = 1
    }
}
