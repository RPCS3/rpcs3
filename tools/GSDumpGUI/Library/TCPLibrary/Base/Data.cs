/*
* Copyright (c) 2009 Ferreri Alessio
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

using System;
using System.Collections.Generic;
using System.Text;

namespace TCPLibrary.Core
{
    /// <summary>
    /// Structure for containing the data to be sent over a base client/server
    /// </summary>
    public class Data
    {
        /// <summary>
        /// Data to be sent.
        /// </summary>
        private byte[] _message;

        /// <summary>
        /// Get/set the data to be sent.
        /// </summary>
        public byte[] Message
        {
            get { return _message; }
            set { _message = value; }
        }

        /// <summary>
        /// Base constructor of the class.
        /// </summary>
        /// <param name="msg">Data to be sent.</param>
        public Data(byte[] msg)
        {
            this._message = msg;
        }
    }
}
