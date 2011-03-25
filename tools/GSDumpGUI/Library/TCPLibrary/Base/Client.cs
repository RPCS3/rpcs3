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
using System.Net;
using System.Threading;
using System.IO;
using System.ComponentModel;
using System.Text;
using System.Diagnostics;

namespace TCPLibrary.Core
{
    /// <summary>
    /// Base TCP client class wrapped around TcpClient.
    /// </summary>
    public class Client
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
        /// Address to which the client is connected.
        /// </summary>
        private IPEndPoint _address;

        /// <summary>
        /// Flag to permit thread exit from the external.
        /// </summary>
        protected Boolean _active;
        /// <summary>
        /// Socket maintaining the connection.
        /// </summary>
        protected TcpClient _socket;

        /// <summary>
        /// Get/set the address to which the client is connected.
        /// </summary>
        public IPEndPoint Address
        {
            get { return _address; }
        }

        /// <summary>
        /// Get the state of the connection.
        /// </summary>
        public Boolean Connected
        {
            get { return _socket != null; }
        }

        /// <summary>
        /// Delegate for the event of receiving/sending a line of data from/to the server.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        /// <param name="Data">Line of data received.</param>
        public delegate void DataCommunicationHandler(Client sender, Data Data);
        /// <summary>
        /// Occurs when a line of data is received from the server.
        /// </summary>
        public event DataCommunicationHandler OnDataReceived;
        /// <summary>
        /// Occurs before the client send a line of data to the server.
        /// </summary>
        public event DataCommunicationHandler OnBeforeDataSent;
        /// <summary>
        /// Occurs after the client send a line of data to the server.
        /// </summary>
        public event DataCommunicationHandler OnAfterDataSent;

        /// <summary>
        /// Delegate for the event of connection/disconnection to the server.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        public delegate void ConnectionStateHandler(Client sender);
        /// <summary>
        /// Occurs when the client successfully connect the server.
        /// </summary>
        public event ConnectionStateHandler OnConnected;
        /// <summary>
        /// Occurs when the client is disconnected from the server.
        /// </summary>
        public event ConnectionStateHandler OnDisconnected;

        /// <summary>
        /// Delegate for the event of connection failed for rejection by the server.
        /// </summary>
        /// <param name="sender">Sender of the event.</param>
        /// <param name="Message">Message of fail sent by the server.</param>
        public delegate void ConnectionFailedHandler(Client sender, byte[] Message);
        /// <summary>
        /// Occurs when the client failed to connect to the server because the server rejected the connection.
        /// </summary>
        public event ConnectionFailedHandler OnConnectFailed;

        /// <summary>
        /// Base constructor of the class.
        /// </summary>
        public Client()
        {
            _lock = new object();
            _address = null;
        }

        /// <summary>
        /// Try to connect to the server specified.
        /// </summary>
        /// <param name="Indirizzo">Address of the server to connect.</param>
        /// <param name="Port">Port of the server to connect.</param>
        /// <returns>True if the connection is successfull, false otherwise.</returns>
        /// <exception cref="TCPLibrary.Core.AlreadyConnectedException" />
        /// <exception cref="System.Net.Sockets.SocketException" />
        public virtual Boolean Connect(String Indirizzo, Int32 Port)
        {
            if (!Connected)
            {
                IPHostEntry addr = Dns.GetHostEntry(Indirizzo);
                IPAddress ip = null;

                foreach (var itm in addr.AddressList)
                {
                    if (itm.AddressFamily == AddressFamily.InterNetwork)
                    {
                        ip = itm;
                        break;
                    }
                }

                if (ip != null)
                {
                    _address = new IPEndPoint(ip, Port);
                    _socket = new TcpClient();

                    try
                    {
                        _socket.Connect(_address);
                    }
                    catch (SocketException)
                    {
                        _socket = null;
                        _address = null;
                        throw;
                    }

                    tr = new BinaryReader(_socket.GetStream());
                    tw = new BinaryWriter(_socket.GetStream());

                    // Receive the confirmation of the status of the connection to the server.
                    // Is CONNECTEDTCPSERVER if the connection is successfull, all other cases are wrong.

                    Int32 Length = Convert.ToInt32(tr.ReadInt32());
                    byte[] arr = new byte[Length];
                    tr.Read(arr, 0, Length);
                    ASCIIEncoding ae = new ASCIIEncoding();
                    String Accept = ae.GetString(arr, 0, arr.Length);

                    if (Accept == "CONNECTEDTCPSERVER")
                    {
                        _active = true;

                        Thread thd = new Thread(new ThreadStart(MainThread));
                        thd.IsBackground = true;
                        thd.Name = "Client connected to " + Indirizzo + ":" + Port.ToString();
                        thd.Start();

                        if (OnConnected != null)
                            OnConnected(this);

                        return true;
                    }
                    else
                    {
                        Stop();
                        if (OnConnectFailed != null)
                            OnConnectFailed(this, arr);

                        return false;
                    }
                }
                else
                    return false;
            }
            else
                throw new ArgumentException("The client is already connected!");
        }

        /// <summary>
        /// Disconnect a Client if connected.
        /// </summary>
        public virtual void Disconnect()
        {
            lock (_lock)
            {
                _active = false;
                tr.Close();
                tw.Close();
            }
        }

        /// <summary>
        /// Disconnect a Client if connected.
        /// </summary>
        protected virtual void Stop()
        {
            if (_socket != null)
            {
                tr.Close();
                tw.Close();
                _socket.Close();

                _socket = null;
                _address = null;

                if (OnDisconnected != null)
                    OnDisconnected(this);
            }
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
                catch (Exception) { }

                lock (_lock)
                {
                    if (_active)
                    {
                        Boolean Stato = _socket.Client.Poll(100, SelectMode.SelectRead);
                        if ((arr == null) && (Stato == true))
                            break;
                        else
                            if (OnDataReceived != null)
                                OnDataReceived(this, new Data(arr));
                    }
                    else
                        break;
                }
            }
            Stop();
        }

        /// <summary>
        /// Send a line of data to the server.
        /// </summary>
        /// <param name="msg">Data to send to the server.</param>
        /// <exception cref="TCPLibrary.Core.NotConnectedException" />
        public void Send(Data msg)
        {
            if (_active)
            {
                if (OnBeforeDataSent != null)
                    OnBeforeDataSent(this, msg);
                try
                {
                    tw.Write(msg.Message.Length);
                    tw.Write(msg.Message);
                    tw.Flush();

                    if (OnAfterDataSent != null)
                        OnAfterDataSent(this, msg);
                }
                catch (IOException)
                {
                    // Pensare a cosa fare quà. Questo è il caso in cui il server ha chiuso forzatamente 
                    // la connessione mentre il client mandava roba.
                }
            }
            else
                throw new ArgumentException("The link is closed. Unable to send data.");
        }
    }
}
