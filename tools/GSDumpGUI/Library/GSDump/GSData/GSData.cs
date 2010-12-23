using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GSData
    {
        public GSType id;
        public byte[] data;
    }

    public enum GSType
    {
        Transfer = 0,
        VSync = 1,
        ReadFIFO2 = 2,
        Registers = 3
    }
}
