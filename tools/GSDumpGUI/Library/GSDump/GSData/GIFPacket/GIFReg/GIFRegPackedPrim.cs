using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegPackedPrim : GIFReg
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
