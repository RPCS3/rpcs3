using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFTag
    {
        public delegate GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat);
        static public Dictionary<int, Unpack> Registers;

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
            Registers = new Dictionary<int, Unpack>();
            Registers.Add((int)GIFRegDescriptor.PRIM, new Unpack(GIFRegPrim.Unpack));
            Registers.Add((int)GIFRegDescriptor.ST, new Unpack(GIFRegST.Unpack));
            Registers.Add((int)GIFRegDescriptor.TEX0_1, new Unpack(GIFRegTEX0.Unpack));
            Registers.Add((int)GIFRegDescriptor.TEX0_2, new Unpack(GIFRegTEX0.Unpack));
            Registers.Add((int)GIFRegDescriptor.XYZ2, new Unpack(GIFRegXYZ.Unpack));
            Registers.Add((int)GIFRegDescriptor.XYZ3, new Unpack(GIFRegXYZ.Unpack));
            Registers.Add((int)GIFRegDescriptor.XYZF2, new Unpack(GIFRegXYZF.Unpack));
            Registers.Add((int)GIFRegDescriptor.XYZF3, new Unpack(GIFRegXYZF.Unpack));
            Registers.Add((int)GIFRegDescriptor.FOG, new Unpack(GIFRegFOG.Unpack));
            Registers.Add((int)GIFRegDescriptor.UV, new Unpack(GIFRegUV.Unpack));
            Registers.Add((int)GIFRegDescriptor.RGBAQ, new Unpack(GIFRegRGBAQ.Unpack));

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
                    UInt64 HighData = BitConverter.ToUInt64(data, (16 + (i * 16) + 8 + (j * 16 * registers.Count)));

                    switch (t.flg)
                    {
                        case GIFFLG.GIF_FLG_PACKED:
                            try
                            {
                                if (registers[i] == GIFRegDescriptor.AD)
                                {
                                    int destaddr = (int)(HighData & 0xF);
                                    t.regs.Add(Registers[destaddr].Invoke(LowData, HighData, false));
                                }
                                else
                                    t.regs.Add(Registers[(int)registers[i]].Invoke(LowData, HighData, false));
                            }
                            catch (Exception)
                            {
                                // For now till we get all regs implemented
                            }
                            break;
                        case GIFFLG.GIF_FLG_REGLIST:
                            t.regs.Add(Registers[(int)registers[i]].Invoke(LowData, HighData, true));
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
