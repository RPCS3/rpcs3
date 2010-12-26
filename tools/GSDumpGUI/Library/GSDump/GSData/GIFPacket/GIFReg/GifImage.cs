using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GifImage : IGifData
    {
        public byte[] Data;

        public override string ToString()
        {
            return "IMAGE@" + Data.Length.ToString() + " bytes";
        }
    }
}
