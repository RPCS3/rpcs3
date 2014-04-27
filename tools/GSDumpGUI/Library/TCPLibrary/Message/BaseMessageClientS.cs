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

using System.Net.Sockets;
using TCPLibrary.Core;
using TCPLibrary.MessageBased.Core;

namespace TCPLibrary.MessageBased.Core
{
    /// <summary>
    /// Class that manages the single connection between a client and the server based
    /// on Message structures.
    /// </summary>
    public class BaseMessageClientS : ClientS
    {
        /// <summary>
        /// Base constructor of the class.
        /// </summary>
        /// <param name="server">Server to which this client is linked to.</param>
        /// <param name="client">Socket of the client.</param>
        protected internal BaseMessageClientS(Server server, TcpClient client)
            : base(server, client)
        {

        }

        /// <summary>
        /// Send a Message structure to the client.
        /// </summary>
        /// <param name="msg">Message to be sent.</param>
        /// <exception cref="TCPLibrary.Core.NotConnectedException" />
        public void Send(TCPMessage msg)
        {
            ((BaseMessageServer)_server).RaiseBeforeMessageSentEvent(this, msg);
            base.Send(new Data(msg.ToByteArray()));
        }
    }
}
