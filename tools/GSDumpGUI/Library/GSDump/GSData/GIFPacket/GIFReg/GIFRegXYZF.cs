using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegXYZF : GIFReg
    {
        public double X;
        public double Y;
        public UInt32 Z;
        public UInt16 F;

        public bool IsXYZF;

        static public GIFReg UnpackXYZ(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegXYZF xyzf = new GIFRegXYZF();

            xyzf.IsXYZF = false;
            if (PackedFormat && addr == (int)GIFRegDescriptor.XYZ2 && GetBit(HighData, 47, 1) == 1)
                xyzf.Descriptor = GIFRegDescriptor.XYZ3;
            else
                xyzf.Descriptor = (GIFRegDescriptor)addr;

            if (PackedFormat)
            {
                xyzf.X = GetBit(LowData, 0, 16) / 16d;
                xyzf.Y = GetBit(LowData, 32, 16) / 16d;
                xyzf.Z = (UInt32)(GetBit(HighData, 0, 32));
            }
            else
            {
                xyzf.X = GetBit(LowData, 0, 16) / 16d;
                xyzf.Y = GetBit(LowData, 16, 16) / 16d;
                xyzf.Z = (UInt32)(GetBit(LowData, 32, 32));
            }
            return xyzf;
        }

        static public GIFReg Unpack(GIFTag tag, int addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            GIFRegXYZF xyzf = new GIFRegXYZF();

            xyzf.IsXYZF = true;
            if (PackedFormat && addr == (int)GIFRegDescriptor.XYZF2 && GetBit(HighData, 47, 1) == 1)
                xyzf.Descriptor = GIFRegDescriptor.XYZF3;
            else
                xyzf.Descriptor = (GIFRegDescriptor)addr;

            if (PackedFormat)
            {
                xyzf.X = GetBit(LowData, 0, 16) / 16d;
                xyzf.Y = GetBit(LowData, 32, 16) / 16d;
                xyzf.Z = (UInt32)(GetBit(HighData, 4, 24));
                xyzf.F = (UInt16)(GetBit(HighData, 36, 8));
            }
            else
            {
                xyzf.X = GetBit(LowData, 0, 16) / 16d;
                xyzf.Y = GetBit(LowData, 16, 16) / 16d;
                xyzf.Z = (UInt32)(GetBit(LowData, 32, 24));
                xyzf.F = (UInt16)(GetBit(LowData, 56, 8));
            }
            return xyzf;
        }

        public override string ToString()
        {
            return Descriptor.ToString() + "@X : " + X.ToString("F4") + "@Y : " + Y.ToString("F4") + "@Z : " + Z.ToString() + (IsXYZF ? "@F : " + F.ToString() : "");
        }
    }
}
