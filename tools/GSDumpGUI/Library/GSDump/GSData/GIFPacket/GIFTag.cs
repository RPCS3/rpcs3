using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFTag
    {
        public Int32 nloop;
        public Int32 eop;
        public Int32 _pad1;
        public Int32 _pad2;
        public Int32 pre;
        public GIFPrim prim;
        public GIFFLG flg;
        public Int32 nreg;
        public List<IGifData> regs;

        static internal unsafe GIFTag ExtractGifTag(byte[] data)
        {
            Int16 nloopEOP = 0;
            Int16 pad1 = 0;
            Int32 pad2PrePrimFlgNReg = 0;
            Int64 regs = 0;

            nloopEOP = BitConverter.ToInt16(data, 0);
            pad1 = BitConverter.ToInt16(data, 2);
            pad2PrePrimFlgNReg = BitConverter.ToInt32(data, 4);
            regs = BitConverter.ToInt64(data, 8);

            GIFTag t = new GIFTag();
            t.nloop = (nloopEOP & 0x7FFF);
            t.eop = (nloopEOP & 0x8000) >> 15;
            t._pad1 = pad1;
            t._pad2 = (pad2PrePrimFlgNReg & 0x00003FFF);
            t.pre = (pad2PrePrimFlgNReg & 0x00004000) >> 14;

            int prim = (pad2PrePrimFlgNReg & 0x03FF8000) >> 15;
            GIFPrim primm = GIFPrim.ExtractGIFPrim(prim);
            t.prim = primm;

            t.flg = (GIFFLG)((pad2PrePrimFlgNReg & 0xC000000) >> 26);
            t.nreg = (int)(pad2PrePrimFlgNReg & 0xF0000000) >> 28;

            List<GIFRegDescriptor> registers = new List<GIFRegDescriptor>();

            t.regs = new List<IGifData>();
            for (int i = 0; i < t.nreg; i++)
                registers.Add((GIFRegDescriptor)((regs & (Convert.ToInt32(Math.Pow(16, i + 1)) - 1)) >> i * 4));

            for (int j = 0; j < t.nloop; j++)
            {
                for (int i = 0; i < registers.Count; i++)
                {
                    UInt64 LowData = BitConverter.ToUInt64(data, (16 + (i * 16) + (j * 16 * registers.Count)));
                    UInt64 HighData = BitConverter.ToUInt64(data, (16 + (i * 16) + (j * 16 * registers.Count)));

                    switch (t.flg)
                    {
                        case GIFFLG.GIF_FLG_PACKED:
                            switch (registers[i])
                            {
                                case GIFRegDescriptor.PRIM:
                                    if (t.pre == 1)
                                    {
                                        GIFRegPackedPrim pr = new GIFRegPackedPrim();
                                        pr.Descriptor = registers[i];
                                        pr.PrimitiveType = (GS_PRIM)(LowData & 0x7);
                                        pr.IIP = (GSIIP)((LowData & 0x8) >> 3);
                                        pr.TME = Convert.ToBoolean(((LowData & 0x10) >> 4));
                                        pr.FGE = Convert.ToBoolean(((LowData & 0x20) >> 5));
                                        pr.ABE = Convert.ToBoolean(((LowData & 0x40) >> 6));
                                        pr.AA1 = Convert.ToBoolean(((LowData & 0x80) >> 7));
                                        pr.FST = (GSFST)((LowData & 0x100) >> 8);
                                        pr.CTXT = (GSCTXT)((LowData & 0x200) >> 9);
                                        pr.FIX = (GSFIX)((LowData & 0x400) >> 10);
                                        t.regs.Add(pr);
                                    }
                                    break;
                                case GIFRegDescriptor.RGBAQ:
                                    GIFRegPackedRGBAQ r = new GIFRegPackedRGBAQ();
                                    r.Descriptor = registers[i];
                                    r.R = (int)(LowData & 0xFF);
                                    r.G = (int)((LowData & 0xFF00000000) >> 32);
                                    r.B = (int)((HighData & 0xFF));
                                    r.A = (int)((HighData & 0xFF00000000) >> 32);
                                    t.regs.Add(r);
                                    break;
                                case GIFRegDescriptor.ST:
                                    GIFRegPackedST st = new GIFRegPackedST();
                                    st.Descriptor = registers[i];

                                    ulong pt = ((LowData & 0xFFFFFFFF));
                                    void* ptt = &pt;
                                    st.S = *(float*)ptt;

                                    pt = ((LowData & 0xFFFFFFFF00000000) >> 32);
                                    ptt = &pt;
                                    st.T = *(float*)ptt;

                                    pt = ((HighData & 0xFFFFFFFF));
                                    ptt = &pt;
                                    st.Q = *(float*)ptt;

                                    t.regs.Add(st);
                                    break;
                                case GIFRegDescriptor.UV:
                                    GIFRegPackedUV u = new GIFRegPackedUV();
                                    u.Descriptor = registers[i];
                                    u.U = (LowData & 0x3FFF) / 16d;
                                    u.V = ((LowData & 0x3FFF00000000) >> 32) / 16d;

                                    t.regs.Add(u);
                                    break;
                                case GIFRegDescriptor.XYZF2:
                                    GIFRegPackedXYZF xf = new GIFRegPackedXYZF();
                                    xf.Descriptor = registers[i];
                                    xf.X = (LowData & 0xFFFF) / 16d;
                                    xf.Y = ((LowData & 0xFFFF00000000) >> 32) / 16d;
                                    xf.Z = (UInt32)((HighData & 0xFFFFFF0) >> 4);
                                    xf.F = (UInt16)((HighData & 0xFF000000000) >> 36);
                                    xf.ADC = ((HighData & 0x1000000000000) >> 46) == 1;
                                    t.regs.Add(xf);
                                    break;
                                case GIFRegDescriptor.XYZ2:
                                    GIFRegPackedXYZ xf2 = new GIFRegPackedXYZ();
                                    xf2.Descriptor = registers[i];
                                    xf2.X = (LowData & 0xFFFF) / 16d;
                                    xf2.Y = ((LowData & 0xFFFF00000000) >> 32) / 16d;
                                    xf2.Z = (UInt32)((HighData & 0xFFFFFF0) >> 4);
                                    xf2.ADC = ((HighData & 0x1000000000000) >> 46) == 1;
                                    t.regs.Add(xf2);
                                    break;
                                case GIFRegDescriptor.FOG:
                                    break;
                                case GIFRegDescriptor.Reserved:
                                    break;
                                case GIFRegDescriptor.XYZF3:
                                    GIFRegPackedXYZF xf3 = new GIFRegPackedXYZF();
                                    xf3.Descriptor = registers[i];
                                    xf3.X = (LowData & 0xFFFF) / 16d;
                                    xf3.Y = ((LowData & 0xFFFF00000000) >> 32) / 16d;
                                    xf3.Z = (UInt32)((HighData & 0xFFFFFF0) >> 4);
                                    xf3.F = (UInt16)((HighData & 0xFF000000000) >> 36);
                                    xf3.ADC = ((HighData & 0x1000000000000) >> 46) == 1;
                                    t.regs.Add(xf3);
                                    break;
                                case GIFRegDescriptor.XYZ3:
                                    GIFRegPackedXYZ xf4 = new GIFRegPackedXYZ();
                                    xf4.Descriptor = registers[i];
                                    xf4.X = (LowData & 0xFFFF) / 16d;
                                    xf4.Y = ((LowData & 0xFFFF00000000) >> 32) / 16d;
                                    xf4.Z = (UInt32)((HighData & 0xFFFFFF0) >> 4);
                                    xf4.ADC = ((HighData & 0x1000000000000) >> 46) == 1;
                                    t.regs.Add(xf4);
                                    break;
                                case GIFRegDescriptor.AD:
                                    int destaddr = (int)(HighData & 0xF);
                                    
                                    break;
                                case GIFRegDescriptor.NOP:

                                    break;
                                default:
                                    break;
                            }
                            break;
                        case GIFFLG.GIF_FLG_REGLIST:
                            // TODO : 
                            break;
                        case GIFFLG.GIF_FLG_IMAGE:
                            GifImage image = new GifImage();
                            image.LowData = BitConverter.GetBytes(LowData);
                            image.HighData = BitConverter.GetBytes(HighData);
                            t.regs.Add(image);
                            break;
                        case GIFFLG.GIF_FLG_IMAGE2:
                            GifImage image2 = new GifImage();
                            image2.LowData = BitConverter.GetBytes(LowData);
                            image2.HighData = BitConverter.GetBytes(HighData);
                            t.regs.Add(image2);
                            break;
                        default:
                            break;
                    }
                }
            }
            return t;
        }
    }

    public enum GIFFLG
    {
        GIF_FLG_PACKED =0,
        GIF_FLG_REGLIST =1,
        GIF_FLG_IMAGE = 2,
        GIF_FLG_IMAGE2 = 3
    }
}
