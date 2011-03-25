using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    [Serializable]
    public class GIFTag : GIFUtil
    {
        public delegate GIFReg Unpack(GIFTag tag, byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat);
        static public Dictionary<int, Unpack> UnpackReg;

        private UInt64 TAG, REGS;
        internal float Q; // GIF has an internal Q register which is reset to 1.0 at the tag and updated on packed ST(Q) for output at next RGBAQ

        public GSTransferPath path;
        public UInt32 nloop;
        public UInt32 eop;
        public UInt32 pre;
        public GIFPrim prim;
        public GIFFLG flg;
        public UInt32 nreg;
        public List<IGifData> regs;
        public int size;
        
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

        static internal GIFTag ExtractGifTag(byte[] data, GSTransferPath path)
        {
            GIFTag t = new GIFTag();
            t.size = data.Length;
            t.path = path;
            t.TAG = BitConverter.ToUInt64(data, 0);
            t.REGS = BitConverter.ToUInt64(data, 8);

            t.Q = 1f;
            t.nloop = (uint)GetBit(t.TAG, 0, 15);
            t.eop = (uint)GetBit(t.TAG, 15, 1);
            t.pre = (uint)GetBit(t.TAG, 46, 1);
            t.prim = GIFPrim.ExtractGIFPrim((uint)GetBit(t.TAG, 47, 11));
            t.flg = (GIFFLG)GetBit(t.TAG, 58, 2);
            t.nreg = (uint)GetBit(t.TAG, 60, 4);
            if (t.nreg == 0)
                t.nreg = 16;

            byte[] registers = new byte[t.nreg];
            Unpack[] regsunpack = new Unpack[t.nreg];

            t.regs = new List<IGifData>();
            for (byte i = 0; i < t.nreg; i++)
            {
                byte reg = (byte)GetBit(t.REGS, i * 4, 4);
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
