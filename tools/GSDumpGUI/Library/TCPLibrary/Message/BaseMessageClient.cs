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

using TCPLibrary.MessageBased.Core;
using TCPLibrary.Core;
using System;

namespace TCPLibrary.MessageBased.Core
{
    /// <summary>
    /// TCP Client Class that work with Message structures.
    /// </summary>
    public class BaseMessageClient : Client
    {
        /// <summary>
        /// Delegate for the event of receiving a message structure from the server.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        /// <param name="Mess">Message received.</param>
        public delegate void MessageReceivedHandler(Client sender, TCPMessage Mess);
        /// <summary>
        /// Occurs when the client receive a message structure from the server.
        /// </summary>
        public event MessageReceivedHandler OnMessageReceived;
        /// <summary>
        /// Delegate for the event of sending a message structure to the server.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        /// <param name="Mess">Message sent.</param>
        public delegate void MessageSentHandler(Client sender, TCPMessage Mess);
        /// <summary>
        /// Occurs before the client send a message structure to the server.
        /// </summary>
        public event MessageSentHandler OnBeforeMessageSent;
        /// <summary>
        /// Occurs after the client send a message structure to the server.
        /// </summary>
        public event MessageSentHandler OnAfterMessageSent;
        /// <summary>
        /// Delegate for the event of connection fail for max users number reached.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        public delegate void MaxUsersReached(Client sender);
        /// <summary>
        /// Occurs when the connection fail as the server reached the maximum number of clients allowed.
        /// </summary>
        public event MaxUsersReached OnMaxUsersConnectionFail;

        /// <summary>
        /// Base constructor of the class.
        /// </summary>
        public BaseMessageClient()
        {
            OnDataReceived += new DataCommunicationHandler(BaseMessageClient_OnDataReceived);
            OnAfterDataSent += new DataCommunicationHandler(BaseMessageClient_OnDataSent);
            OnConnectFailed += new ConnectionFailedHandler(BaseMessageClient_OnConnectFailed);
        }

        /// <summary>
        /// When the connection is rejected by the server raise the correct event.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        /// <param name="Message">Message of the server.</param>
        void BaseMessageClient_OnConnectFailed(Client sender, byte[] Message)
        {
            if (TCPLibrary.MessageBased.Core.TCPMessage.FromByteArray(Message).MessageType == MessageType.MaxUsers)
                if (OnMaxUsersConnectionFail != null)
                    OnMaxUsersConnectionFail(sender);
        }

        /// <summary>
        /// Parse the raw data sent to the server and create Message structures.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        /// <param name="Data">Line of data sent.</param>
        void BaseMessageClient_OnDataSent(Client sender, Data Data)
        {
            TCPMessage msg = TCPMessage.FromByteArray(Data.Message);
            if (OnAfterMessageSent != null)
                OnAfterMessageSent(sender, msg);
        }

        /// <summary>
        /// Parse the raw data received from the server and create Message structures.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        /// <param name="Data">Line of data received.</param>
        void BaseMessageClient_OnDataReceived(Client sender, Data Data)
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
                if (OnMessageReceived != null)
                    OnMessageReceived(sender, msg);
        }

        /// <summary>
        /// Send a message structure to the server.
        /// </summary>
        /// <param name="msg">Message structure to be send.</param>
        /// <exception cref="TCPLibrary.Core.NotConnectedException"></exception>
        public void Send(TCPMessage msg)
        {
            if (OnBeforeMessageSent != null)
                OnBeforeMessageSent(this, msg);
            base.Send(new Data(msg.ToByteArray()));
        }
    }
}
