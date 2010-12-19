using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace GSDumpGUI
{
    public class GSDump
    {
        public Int32 CRC;
        public byte[] StateData;
        public byte[] Registers; // 8192 bytes

        public List<GSData> Data;

        public GSDump()
        {
            Data = new List<GSData>();
        }

        static public GSDump LoadDump(String FileName)
        {
            GSDump dmp = new GSDump();

            BinaryReader br = new BinaryReader(System.IO.File.Open(FileName, FileMode.Open));
            dmp.CRC = br.ReadInt32();
            Int32 ss = br.ReadInt32();
            dmp.StateData = br.ReadBytes(ss);
            dmp.Registers = br.ReadBytes(8192);

            while (br.PeekChar() != -1)
            {
                GSData data = new GSData();
                data.id = (GSType)br.ReadByte();

                switch (data.id)
                {
                    case GSType.Transfer:
                        byte index = br.ReadByte();
                        if (index == 0)
                        {
                            Int32 size = br.ReadInt32();

                            byte[] dat = new byte[16384];
                            List<byte> Data = new List<byte>();
                            Data.Add(index);
                            Data.AddRange(BitConverter.GetBytes(size));
                            Data.AddRange(dat);

                            int startaddr = 16389 - size;

                            for (int i = 0; i < size; i++)
                               Data[startaddr + i] = br.ReadByte();
                            data.data = Data.ToArray();
                        }
                        else
                        {
                            Int32 size = br.ReadInt32();

                            List<byte> Data = new List<byte>();
                            Data.Add(index);
                            Data.AddRange(br.ReadBytes(size));
                            data.data = Data.ToArray();
                        }
                        break;
                    case GSType.VSync:
                        data.data = br.ReadBytes(1);
                        break;
                    case GSType.ReadFIFO2:
                        Int32 sF = br.ReadInt32();
                        data.data = br.ReadBytes(sF);
                        break;
                    case GSType.Registers:
                        data.data = br.ReadBytes(8192);
                        break;
                }
                dmp.Data.Add(data);
            }
            br.Close();

            return dmp;
        }
    }
}
