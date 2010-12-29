using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    [Serializable]
    public class GIFPrim : GIFUtil
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

        static internal GIFPrim ExtractGIFPrim(UInt32 LowData)
        {
            GIFPrim pr = new GIFPrim();
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
            return "Primitive Type : " + PrimitiveType.ToString() + "@IIP : " + IIP.ToString() + "@TME : " + TME.ToString() + "@FGE : " + FGE.ToString()
                + "@ABE : " + ABE.ToString() + "@AA1 : " + AA1.ToString() + "@FST : " + FST.ToString() + "@CTXT : " + CTXT.ToString() + "@FIX : " + FIX.ToString();
        }
    }

    public enum GS_PRIM
    {
        GS_POINTLIST = 0,
        GS_LINELIST = 1,
        GS_LINESTRIP = 2,
        GS_TRIANGLELIST = 3,
        GS_TRIANGLESTRIP = 4,
        GS_TRIANGLEFAN = 5,
        GS_SPRITE = 6,
        GS_INVALID = 7,
    }
}
