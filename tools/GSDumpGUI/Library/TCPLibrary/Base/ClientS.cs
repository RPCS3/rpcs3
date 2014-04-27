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
using System.Net.Sockets;
using System.Threading;
using System.IO;
using System.Diagnostics;

namespace TCPLibrary.Core
{
    /// <summary>
    /// Base class that manages the single connection between a client and the server.
    /// </summary>
    public class ClientS
    {
        /// <summary>
        /// Lock object to assure that certain operation over the socket class are executed 
        /// in an exclusive way.
        /// </summary>
        private Object _lock;

        /// <summary>
        /// Wrapper around the Network Stream of the socket. (Read-Only)
        /// </summary>
        private BinaryReader tr;
        /// <summary>
        /// Wrapper around the Network Stream of the socket. (Write-Only)
        /// </summary>
        private BinaryWriter tw;
        /// <summary>
        /// Current IP address of the client.
        /// </summary>
        private String _ipaddress;

        /// <summary>
        /// Flag to permit thread exit from the external.
        /// </summary>
        protected Boolean _active;
        /// <summary>
        /// Link to the server to which this client is connected.
        /// </summary>
        protected Server _server;
        /// <summary>
        /// Actual socket of the client.
        /// </summary>
        protected TcpClient _client;

        /// <summary>
        /// Get the state of the connection.
        /// </summary>
        public Boolean Connected
        {
            get { return _client != null; }
        }

        /// <summary>
        /// IP Address of the client.
        /// </summary>
        public String IPAddress
        {
            get { return _ipaddress; }
        }

        /// <summary>
        /// Base class constructor.
        /// </summary>
        /// <param name="server">Server to which this client is linked to.</param>
        /// <param name="client">Socket of the client.</param>
        protected internal ClientS(Server server, TcpClient client)
        {
            _lock = new object();

            _active = true;
            _server = server;
            _client = client;
            _ipaddress = _client.Client.RemoteEndPoint.ToString();

            NetworkStream ns = _client.GetStream();
            tr = new BinaryReader(ns);
            tw = new BinaryWriter(ns);
        }

        /// <summary>
        /// Start up the thread managing this Client-Server connection.
        /// </summary>
        protected internal virtual void Start()
        {
            Thread _thread = new Thread(new ThreadStart(MainThread));
            _thread.IsBackground = true;
            _thread.Name = "Thread Client " + _ipaddress;
            _thread.Start();
        }

        /// <summary>
        /// Thread function that actually run the socket work.
        /// </summary>
        private void MainThread()
        {
            while (_active)
            {
                byte[] arr = null;

                try 
                {
                    int length = Convert.ToInt32(tr.ReadInt32());
                    arr = new byte[length];
                    int index = 0;

                    while (length > 0)
                    {

                        int receivedBytes = tr.Read(arr, index, length);
                        length -= receivedBytes;
                        index += receivedBytes;
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex.ToString());
                }

                lock (_lock)
                {
                    if (_active)
                    {
                        Boolean Stato = _client.Client.Poll(100, SelectMode.SelectRead);
                        if ((arr == null) && (Stato == true))
                            break;
                        else
                            _server.RaiseDataReceivedEvent(this, new Data(arr));
                    }
                    else
                        break;
                }
            }
            Stop();
        }

        /// <summary>
        /// Send a line of data to the client.
        /// </summary>
        /// <param name="Data">Data to send to the client.</param>
        /// <exception cref="TCPLibrary.Core.NotConnectedException" />
        public void Send(Data Data)
        {
            if (_active)
            {
                _server.RaiseBeforeDataSentEvent(this, Data);

                try
                {
                    tw.Write(Data.Message.Length);
                    tw.Write(Data.Message);
                    tw.Flush();

                    _server.RaiseAfterDataSentEvent(this, Data);
                }
                catch (Exception ex)
                {
                    Debug.Write(ex.ToString());
                    // Pensare a cosa fare quà. Questo è il caso in cui il client ha chiuso forzatamente 
                    // la connessione mentre il server mandava roba.
                }
            }
            else
                throw new ArgumentException("The link is closed. Unable to send data.");
        }

        /// <summary>
        /// Close the link between Client e Server.
        /// </summary>
        public void Disconnect()
        {
            lock (_lock)
            {
                _active = false;
                tr.Close();
                tw.Close();
            }
        }

        /// <summary>
        /// Close the link between Client e Server.
        /// </summary>
        protected internal void Stop()
        {
            if (_client != null)
            {
                _server.RaiseClientBeforeDisconnectedEvent(this);

                tr.Close();
                tw.Close();

                _client.Close();
                _client = null;

                lock (_server.Clients)
                {
                    _server.Clients.Remove(this);
                }

                _server.RaiseClientAfterDisconnectedEvent(this);
            }
        }
    }
}
