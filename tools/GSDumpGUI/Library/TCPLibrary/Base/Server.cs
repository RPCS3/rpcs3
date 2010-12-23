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
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.IO;
using System.ComponentModel;
using System.Text;

namespace TCPLibrary.Core
{
    /// <summary>
    /// Base TCP server class wrapped around TcpListener.
    /// </summary>
    public class Server
    {
        /// <summary>
        /// Socket maintaining the connection.
        /// </summary>
        private TcpListener _socket;
        /// <summary>
        /// Port to which the server will listen.
        /// </summary>
        private Int32 _port;
        /// <summary>
        /// Whether the server is enabled or not.
        /// </summary>
        private Boolean _enabled;
        /// <summary>
        /// List of the clients connected to the server.
        /// </summary>
        private List<ClientS> _clients;
        /// <summary>
        /// Number of connection permitted in the backlog of the server.
        /// </summary>
        private Int32 _connectionbacklog;

        /// <summary>
        /// Delegate for the event of the Enabled property change.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        public delegate void EnabledChangedHandler(Server sender);
        /// <summary>
        /// Occurs when the Enabled property is changed.
        /// </summary>
        public event EnabledChangedHandler OnEnabledChanged;

        /// <summary>
        /// Delegate for the event of receiving a line of data from a client.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="client">Client involved in the communication.</param>
        /// <param name="Data">Line of data received.</param>
        public delegate void DataCommunicationHandler(Server server, ClientS client, Data Data);
        /// <summary>
        /// Occurs when a client send a line of data to the server.
        /// </summary>
        public event DataCommunicationHandler OnClientDataReceived;
        /// <summary>
        /// Occurs before the server send a line of data to a client.
        /// </summary>
        public event DataCommunicationHandler OnClientBeforeDataSent;
        /// <summary>
        /// Occurs after the server send a line of data to a client.
        /// </summary>
        public event DataCommunicationHandler OnClientAfterDataSent;

        /// <summary>
        /// Delegate for the event of a connection of a client.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="sender">The new client connected.</param>
        public delegate void ConnectedHandler(Server server, ClientS sender);
        /// <summary>
        /// Occurs after a client is connected to the server.
        /// </summary>
        public event ConnectedHandler OnClientAfterConnect;

        /// <summary>
        /// Delegate for the event of a connection of a client.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="client">The new client to be connected.</param>
        /// <param name="args">Specify if the client should be accepted into the server.</param>
        public delegate void BeforeConnectedHandler(Server server, ClientS client, CancelArgs args);
        /// <summary>
        /// Occurs before a client is allowed to connect to the server.
        /// </summary>
        public event BeforeConnectedHandler OnClientBeforeConnect;

        /// <summary>
        /// Delegate for the event of disconnection of a client.
        /// </summary>
        /// <param name="server">Server raising the event.</param>
        /// <param name="sender">The client disconnected.</param>
        public delegate void DisconnectedHandler(Server server, ClientS sender);
        /// <summary>
        /// Occurs right after a client disconnect from the server.
        /// </summary>
        public event DisconnectedHandler OnClientAfterDisconnected;
        /// <summary>
        /// Occurs before a client disconnect from the server.
        /// </summary>
        public event DisconnectedHandler OnClientBeforeDisconnected;

        /// <summary>
        /// Get/set the port number to which the server will listen. Cannot be set while the server is active.
        /// </summary>
        /// <exception cref="TCPLibrary.Core.ServerAttivoException" />
        public Int32 Port
        {
            get { return _port; }
            set
            {
                if (Enabled == false)
                    _port = value;
                else
                    throw new ArgumentException("Impossibile eseguire l'operazione a server attivo");
            }
        }

        /// <summary>
        /// Get/set the enabled state of the server. Setting this to true will actually activate the server.
        /// </summary>
        /// <exception cref="System.Net.Sockets.SocketException" />
        public Boolean Enabled
        {
            get { return _enabled; }
            set
            {
                if (value == true)
                {
                    if (_enabled == false)
                        ActivateServer();
                }
                else
                {
                    if (_enabled == true)
                        DeactivateServer();
                }
            }
        }

        /// <summary>
        /// Get/set the number of connection permitted in the backlog of the server.
        /// </summary>
        /// <exception cref="TCPLibrary.Core.ServerAttivoException" />
        public Int32 ConnectionBackLog
        {
            get { return _connectionbacklog; }
            set
            {
                if (Enabled == false)
                    _connectionbacklog = value;
                else
                    throw new ArgumentException("Impossibile eseguire l'operazione a server attivo");
            }
        }

        /// <summary>
        /// Get the list of the clients connected to the server.
        /// </summary>
        public List<ClientS> Clients
        {
            get { return _clients; }
        }

        /// <summary>
        /// Deactivate the server.
        /// </summary>
        protected virtual void DeactivateServer()
        {
            _enabled = false;
            _socket.Stop();
            _socket = null;

            lock (_clients)
            {
                for (int i = 0; i < _clients.Count; i++)
                    _clients[i].Disconnect();
            }

            if (OnEnabledChanged != null)
                OnEnabledChanged(this);
        }

        /// <summary>
        /// Activate the server.
        /// </summary>
        protected virtual void ActivateServer()
        {
            _socket = new TcpListener(IPAddress.Any, Port);
            _socket.Start(ConnectionBackLog);
            Thread thd = new Thread(new ThreadStart(MainThread));
            thd.Name = "Server on port " + Port.ToString();
            thd.IsBackground = true;
            thd.Start();
            _enabled = true;
            if (OnEnabledChanged != null)
                OnEnabledChanged(this);
        }

        /// <summary>
        /// Broadcast a line of data to all the clients connected to the server.
        /// </summary>
        /// <param name="Data">Line of data to be sent.</param>
        /// <exception cref="TCPLibrary.Core.ServerNonAttivoException" />
        public void Broadcast(Data Data)
        {
            if (Enabled)
            {
                lock (_clients)
                {
                    foreach (var itm in _clients)
                        if (itm.Connected)
                            itm.Send(Data);
                }
            }
            else
                throw new ArgumentException("Unable to execute this operation when the server is inactive.");
        }

        /// <summary>
        /// Base constructor of the class.
        /// </summary>
        public Server()
        {
            _clients = new List<ClientS>();
            _port = 0;
            _connectionbacklog = 0;
            _enabled = false;
        }

        /// <summary>
        /// Thread function that actually run the server socket work.
        /// </summary>
        private void MainThread()
        {
            try
            {
                while (Enabled == true)
                {
                    TcpClient client = _socket.AcceptTcpClient();

                    CancelArgs args = new CancelArgs(false);
                    ClientS cl = CreateClient(client);
                    if (OnClientBeforeConnect != null)
                        OnClientBeforeConnect(this, cl, args);

                    if (args.Cancel != true)
                    {
                        lock (_clients)
                        {
                            _clients.Add(cl);
                        }

                        ASCIIEncoding ae = new ASCIIEncoding();
                        byte[] arr = ae.GetBytes("CONNECTEDTCPSERVER");

                        cl.Send(new Data(arr));

                        if (OnClientAfterConnect != null)
                            OnClientAfterConnect(this, cl);
                        cl.Start();
                    }
                    else
                    {
                        client.GetStream().Close();
                        client.Close();
                    }
                }
            }
            catch (SocketException)
            {
                Enabled = false;
            }
        }

        /// <summary>
        /// Overridable function that create the structure to memorize the client data.
        /// </summary>
        /// <param name="socket">Socket of the client.</param>
        /// <returns>The structure in which memorize all the information of the client.</returns>
        protected virtual ClientS CreateClient(TcpClient socket)
        {
            ClientS cl = new ClientS(this, socket);
            return cl;
        }

        /// <summary>
        /// Raise the OnClientAfterDataSent event.
        /// </summary>
        /// <param name="cl">Client that raised the event.</param>
        /// <param name="data">Line of data sent.</param>
        internal void RaiseAfterDataSentEvent(ClientS cl, Data data)
        {
            if (OnClientAfterDataSent != null)
                OnClientAfterDataSent(this, cl, data);
        }

        /// <summary>
        /// Raise the OnClientBeforeDataSent event.
        /// </summary>
        /// <param name="cl">Client that raised the event.</param>
        /// <param name="data">Line of data sent.</param>
        internal void RaiseBeforeDataSentEvent(ClientS cl, Data data)
        {
            if (OnClientBeforeDataSent != null)
                OnClientBeforeDataSent(this, cl, data);
        }

        /// <summary>
        /// Raise the OnDataReceived event.
        /// </summary>
        /// <param name="cl">Client that raised the event.</param>
        /// <param name="data">Line of data received.</param>
        internal void RaiseDataReceivedEvent(ClientS cl, Data data)
        {
            if (OnClientDataReceived != null)
                OnClientDataReceived(this, cl, data);
        }

        /// <summary>
        /// Raise the OnClientAfterDisconnected event.
        /// </summary>
        /// <param name="cl">Client that raised the event.</param>
        internal void RaiseClientAfterDisconnectedEvent(ClientS cl)
        {
            if (OnClientAfterDisconnected != null)
                OnClientAfterDisconnected(this, cl);
        }

        /// <summary>
        /// Raise the OnClientBeforeDisconnected event.
        /// </summary>
        /// <param name="cl">Client that raised the event.</param>
        internal void RaiseClientBeforeDisconnectedEvent(ClientS cl)
        {
            if (OnClientBeforeDisconnected != null)
                OnClientBeforeDisconnected(this, cl);
        }
    }
}
