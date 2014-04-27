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
using System.Text;
using TCPLibrary.MessageBased.Core;
using System.Net.Sockets;
using System.Threading;
using TCPLibrary.Core;

namespace TCPLibrary.MessageBased.Core
{
    /// <summary>
    /// TCP Server Class that work with Message structures.
    /// </summary>
    public class BaseMessageServer : Server
    {
        /// <summary>
        /// Limit of user allowed inside the server.
        /// </summary>
        protected Int32 _userlimit;

        /// <summary>
        /// Delegate for the event of receiving a Message from a client.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="sender">Client sending the message.</param>
        /// <param name="Mess">Message received.</param>
        public delegate void MessageReceivedHandler(BaseMessageServer server, BaseMessageClientS sender, TCPMessage Mess);
        /// <summary>
        /// Occurs when a Message is received by the server.
        /// </summary>
        public event MessageReceivedHandler OnClientMessageReceived;
        /// <summary>
        /// Delegate for the event of sending a Message to a client.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="receiver">Client that will receive the message.</param>
        /// <param name="Mess">Message to be sent.</param>
        public delegate void MessageSentHandler(BaseMessageServer server, BaseMessageClientS receiver, TCPMessage Mess);
        /// <summary>
        /// Occurs when the server send a Message to a client.
        /// </summary>
        public event MessageSentHandler OnClientBeforeMessageSent;
        /// <summary>
        /// Occurs when the server send a Message to a client.
        /// </summary>
        public event MessageSentHandler OnClientAfterMessageSent;

        /// <summary>
        /// Get/set the limit of users allowed inside the server.
        /// </summary>
        public Int32 UserLimit
        {
            get { return _userlimit; }
            set { _userlimit = value; }
        }

        /// <summary>
        /// Base constructor of the class.
        /// </summary>
        public BaseMessageServer() : base()
        {
            OnClientBeforeConnect += new BeforeConnectedHandler(BaseMessageServer_OnClientBeforeConnect);
            OnClientDataReceived += new DataCommunicationHandler(BaseMessageServer_OnDataReceived);
            OnClientAfterDataSent += new DataCommunicationHandler(BaseMessageServer_OnDataSent);
            _userlimit = 0;
        }

        /// <summary>
        /// Kick the client if the server reached the maximum allowed number of clients.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="client">Client connecting to the server.</param>
        /// <param name="args">Specify if the client should be accepted into the server.</param>
        void BaseMessageServer_OnClientBeforeConnect(Server server, ClientS client, CancelArgs args)
        {
            if ((Clients.Count >= UserLimit) && (UserLimit != 0))
            {
                TCPMessage msg = new TCPMessage();
                msg.MessageType = MessageType.MaxUsers;
                ((BaseMessageClientS)client).Send(msg);

                args.Cancel = true;
            }
        }

        /// <summary>
        /// Trasform the line of data sent into a Message structure and raise
        /// the event linked.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="receiver">Client that will receive the Message.</param>
        /// <param name="Data">Line of data sent.</param>
        void BaseMessageServer_OnDataSent(Server server, ClientS receiver, Data Data)
        {
            TCPMessage msg = null;
            try
            {
                msg = TCPMessage.FromByteArray(Data.Message);
            }
            catch (Exception)
            {

            }
            if (msg != null)
                if (OnClientAfterMessageSent != null)
                    OnClientAfterMessageSent(this, (BaseMessageClientS)receiver, msg);
        }

        /// <summary>
        /// Raise the OnClientBeforeMessageSent event.
        /// </summary>
        /// <param name="cl">Client that raised the event.</param>
        /// <param name="msg">Message to be sent.</param>
        internal void RaiseBeforeMessageSentEvent(ClientS cl, TCPMessage msg)
        {
            if (OnClientBeforeMessageSent != null)
                OnClientBeforeMessageSent(this, (BaseMessageClientS)cl, msg);
        }

        /// <summary>
        /// Trasform the line of data received into a Message structure and raise
        /// the event linked.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="sender">Client sending the data.</param>
        /// <param name="Data">Line of data received.</param>
        void BaseMessageServer_OnDataReceived(Server server, ClientS sender, Data Data)
        {
            TCPMessage msg = null;
            try
            {
                msg = TCPMessage.FromByteArray(Data.Message);
            }
            catch (Exception)
            {

            }
            if (msg != null) 
                if (OnClientMessageReceived != null)
                    OnClientMessageReceived(this, (BaseMessageClientS)sender, msg);
        }

        /// <summary>
        /// Function that create the structure to memorize the client data.
        /// </summary>
        /// <param name="socket">Socket of the client.</param>
        /// <returns>The structure in which memorize all the information of the client.</returns>
        protected override ClientS CreateClient(TcpClient socket)
        {
            return new BaseMessageClientS(this, socket);
        }

        /// <summary>
        /// Send a message to all clients in broadcast.
        /// </summary>
        /// <param name="Data">Message to be sent.</param>
        public void Broadcast(TCPMessage Data)
        {
            base.Broadcast(new Data(Data.ToByteArray()));
        }
    }
}
