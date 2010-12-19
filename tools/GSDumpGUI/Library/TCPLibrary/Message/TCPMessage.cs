/*
The MIT License

Copyright (c) 2008 Ferreri Alessio

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

using System;
using System.Collections.Generic;
using System.Xml.Serialization;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;

namespace TCPLibrary.MessageBased.Core
{
    /// <summary>
    /// Message structure that contains all the information of the message exchanged between
    /// Message driven server/client.
    /// </summary>
    [Serializable]
    public class TCPMessage
    {
        /// <summary>
        /// Message Type.
        /// </summary>
        private MessageType _messageType;
        /// <summary>
        /// Messages parameters.
        /// </summary>
        private List<object> _parameters;

        /// <summary>
        /// Get/set the message type.
        /// </summary>
        public MessageType MessageType
        {
            get { return _messageType; }
            set { _messageType = value; }
        }

        /// <summary>
        /// Get/set the message parameters.
        /// </summary>
        public List<object> Parameters
        {
            get { return _parameters; }
        }

        /// <summary>
        /// Base constructor of the class.
        /// </summary>
        public TCPMessage()
        {
            _messageType = MessageType.Connect;
            _parameters = new List<object>();
        }

        /// <summary>
        /// Parse a string and create a Message structure.
        /// </summary>
        /// <param name="data">Raw data.</param>
        /// <returns>Parsed message structure.</returns>
        static public TCPMessage FromByteArray(byte[] data)
        {
            MemoryStream ms = new MemoryStream();
            BinaryWriter sw = new BinaryWriter(ms);
            sw.Write(data, 0, data.Length);
            sw.Flush();
            ms.Position = 0;

            BinaryFormatter formatter = new BinaryFormatter();
            TCPMessage msg = formatter.Deserialize(ms) as TCPMessage;

            return msg;
        }

        /// <summary>
        /// Trasform the structure into a String.
        /// </summary>
        /// <returns>The structure in a String format.</returns>
        public byte[] ToByteArray()
        {
            MemoryStream ms = new MemoryStream();
            BinaryFormatter formatter = new BinaryFormatter();
            formatter.Serialize(ms, this);
            ms.Position = 0;
            return ms.ToArray();
        }
    }

    public enum MessageType
    {
        Connect,
        MaxUsers,
        SizeDump,
        Statistics
    }
}
