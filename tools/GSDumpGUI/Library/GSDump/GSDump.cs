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

        public int Size
        {
            get
            {
                int size = 0;
                size = 4;
                size += StateData.Length;
                size += Registers.Length;
                foreach (var itm in Data)
                {
                    size += itm.data.Length;
                }
                return size;
            }
        }

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
                GSType id = (GSType)br.ReadByte();
                switch (id)
                {
                    case GSType.Transfer:
                        GSTransfer data = new GSTransfer();

                        byte index = br.ReadByte();

                        data.id = id;
                        data.Path = (GSTransferPath)index;

                        Int32 size = br.ReadInt32();

                        List<byte> Data = new List<byte>();
                        Data.AddRange(br.ReadBytes(size));
                        data.data = Data.ToArray();
                        dmp.Data.Add(data);
                        break;
                    case GSType.VSync:
                        GSData dataV = new GSData();
                        dataV.id = id;
                        dataV.data = br.ReadBytes(1);
                        dmp.Data.Add(dataV);
                        break;
                    case GSType.ReadFIFO2:
                        GSData dataR = new GSData();
                        dataR.id = id;
                        Int32 sF = br.ReadInt32();
                        dataR.data = BitConverter.GetBytes(sF);
                        dmp.Data.Add(dataR);
                        break;
                    case GSType.Registers:
                        GSData dataRR = new GSData();
                        dataRR.id = id;
                        dataRR.data = br.ReadBytes(8192);
                        dmp.Data.Add(dataRR);
                        break;
                    default:
                        break;
                }
            }
            br.Close();

            return dmp;
        }
    }
}
