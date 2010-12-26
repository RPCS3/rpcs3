using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegPRIM : GIFReg
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

        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegPRIM pr = new GIFRegPRIM();
            pr.Descriptor = (GIFRegDescriptor)addr;
            pr.PrimitiveType = (GS_PRIM)GetBit(LowData, 0, 3);
            pr.IIP = (GSIIP)GetBit(LowData, 3, 1);
            pr.TME = Convert.ToBoolean(GetBit(LowData, 4, 1));
            pr.FGE = Convert.ToBoolean(GetBit(LowData, 5, 1));
            pr.ABE = Convert.ToBoolean(GetBit(LowData, 6, 1));
            pr.AA1 = Convert.ToBoolean(GetBit(LowData, 7, 1));
            pr.FST = (GSFST)(GetBit(LowData, 8, 1));
            pr.CTXT = (GSCTXT)(GetBit(LowData, 9, 1));
            pr.FIX = (GSFIX)(GetBit(LowData, 10, 1));
            return pr;
        }

        public override string ToString()
        {
            return Descriptor.ToString() + "@Primitive Type : " + PrimitiveType.ToString() + "@IIP : " + IIP.ToString() + "@TME : " + TME.ToString() + "@FGE : " + FGE.ToString()
                + "@ABE : " + ABE.ToString() + "@AA1 : " + AA1.ToString() + "@FST : " + FST.ToString() + "@CTXT : " + CTXT.ToString() + "@FIX : " + FIX.ToString();
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
