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
            pr.PrimitiveType = (GS_PRIM)GIFReg.GetBit(LowData, 0, 3);
            pr.IIP = (GSIIP)GIFReg.GetBit(LowData, 3, 1);
            pr.TME = Convert.ToBoolean(GIFReg.GetBit(LowData, 4, 1));
            pr.FGE = Convert.ToBoolean(GIFReg.GetBit(LowData, 5, 1));
            pr.ABE = Convert.ToBoolean(GIFReg.GetBit(LowData, 6, 1));
            pr.AA1 = Convert.ToBoolean(GIFReg.GetBit(LowData, 7, 1));
            pr.FST = (GSFST)(GIFReg.GetBit(LowData, 8, 1));
            pr.CTXT = (GSCTXT)(GIFReg.GetBit(LowData, 9, 1));
            pr.FIX = (GSFIX)(GIFReg.GetBit(LowData, 10, 1));
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
