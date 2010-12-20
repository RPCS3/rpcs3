using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GSTransfer : GSData
    {
        public GSTransferPath Path;
    }

    public enum GSTransferPath
    {
        Path1Old = 0,
        Path2 = 1,
        Path3 = 2,
        Path1New = 3
    }
}
