using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFTag
    {
        public delegate GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat);
        static public Dictionary<int, Unpack> UnpackReg;

        public Int32 nloop;
        public Int32 eop;
        public Int32 _pad1;
        public Int32 _pad2;
        public Int32 pre;
        public GIFPrim prim;
        public GIFFLG flg;
        public Int32 nreg;
        public List<IGifData> regs;
        public float Q; // GIF has an internal Q register which is reset to 1.0 at the tag and updated on packed ST(Q) for output at next RGBAQ
        
        static GIFTag()
        {
            UnpackReg = new Dictionary<int, Unpack>();
            UnpackReg.Add((int)GIFRegDescriptor.PRIM, GIFRegPRIM.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.RGBAQ, GIFRegRGBAQ.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.ST, GIFRegST.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.UV, GIFRegUV.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.XYZF2, GIFRegXYZF.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.XYZ2, GIFRegXYZF.UnpackXYZ);
            UnpackReg.Add((int)GIFRegDescriptor.TEX0_1, GIFRegTEX0.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEX0_2, GIFRegTEX0.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.CLAMP_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.CLAMP_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.FOG, GIFRegFOG.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.XYZF3, GIFRegXYZF.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.XYZ3, GIFRegXYZF.UnpackXYZ);
            UnpackReg.Add((int)GIFRegDescriptor.AD, GIFRegAD.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEX1_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEX1_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEX2_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEX2_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.XYOFFSET_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.XYOFFSET_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.PRMODECONT, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.PRMODE, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEXCLUT, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.SCANMSK, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.MIPTBP1_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.MIPTBP1_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.MIPTBP2_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.MIPTBP2_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEXA, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.FOGCOL, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEXFLUSH, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.SCISSOR_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.SCISSOR_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.ALPHA_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.ALPHA_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.DIMX, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.DTHE, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.COLCLAMP, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEST_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TEST_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.PABE, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.FBA_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.FBA_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.FRAME_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.FRAME_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.ZBUF_1, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.ZBUF_2, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.BITBLTBUF, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TRXPOS, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TRXREG, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.TRXDIR, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.HWREG, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.SIGNAL, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.FINISH, GIFRegUnimpl.Unpack);
            UnpackReg.Add((int)GIFRegDescriptor.LABEL, GIFRegUnimpl.Unpack);
        }

        public static Unpack GetUnpack(int reg)
        {
            Unpack ret;
            if (!UnpackReg.TryGetValue(reg, out ret))
                return GIFRegNOP.Unpack;
            return ret;
        }

        static internal GIFTag ExtractGifTag(byte[] data)
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
            t.Q = 1f;
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
            if (t.nreg == 0)
                t.nreg = 16;

            int[] registers = new int[t.nreg];
            Unpack[] regsunpack = new Unpack[t.nreg];

            t.regs = new List<IGifData>();
            for (int i = 0; i < t.nreg; i++)
            {
                int reg = (int)(regs >> (i * 4) & 15);
                registers[i] = reg;
                regsunpack[i] = GetUnpack(reg);
            }

            int p = 16;
            switch (t.flg)
            {
                case GIFFLG.GIF_FLG_PACKED:
                    for (int j = 0; j < t.nloop; j++)
                        for (int i = 0; i < t.nreg; i++)
                        {
                            UInt64 LowData = BitConverter.ToUInt64(data, p);
                            UInt64 HighData = BitConverter.ToUInt64(data, p + 8);
                            t.regs.Add(regsunpack[i](t, registers[i], LowData, HighData, true));
                            p += 16;
                        }
                    break;
                case GIFFLG.GIF_FLG_REGLIST:
                    for (int j = 0; j < t.nloop; j++)
                        for (int i = 0; i < t.nreg; i++)
                        {
                            UInt64 Data = BitConverter.ToUInt64(data, p);
                            t.regs.Add(regsunpack[i](t, registers[i], Data, 0, false));
                            p += 8;
                        }
                    break;
                case GIFFLG.GIF_FLG_IMAGE:
                case GIFFLG.GIF_FLG_IMAGE2:
                    GifImage image = new GifImage();
                    image.Data = new byte[t.nloop * 16];
                    try
                    {
                        Array.Copy(data, 16, image.Data, 0, t.nloop * 16);
                    }
                    catch (ArgumentException) { }
                    t.regs.Add(image);
                    break;
                default:
                    break;
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
