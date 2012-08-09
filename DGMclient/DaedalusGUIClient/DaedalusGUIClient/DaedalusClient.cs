/* $Id: DaedalusClient.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: This class maintains a connection to a Daedalus server.  It has
 * callbacks to alert its parent of new data and (dis)connection events.  It
 * also provides a write method to send data to the server.
 *
 * Copyright (c) 2010-2011, Team Daedalus (Mathew Bergt, Jason Buck, Ken Kelley, and
 * Justin Weaver).
 *
 * Distributed under the BSD-new license. For details see the BSD_LICENSE file
 * that should have been included with this distribution. If the source you
 * acquired this distribution from incorrectly removed this file, the license
 * may be viewed at http://www.opensource.org/licenses/bsd-license.php.
 */

using System;
using System.Collections.Generic;

using System.Net;
using System.Net.Sockets;
using System.Text;

namespace DaedalusGUIClient
{
    // Called when a connect/disconnect event happens.
    delegate void DaedalusClient_ConnectCallback();

    // Called when a new message is received.
    delegate void DaedalusClient_NewMessageCallback(string msg);

    class DaedalusClient
    {
        // The handler to call with new data.
        private DaedalusClient_NewMessageCallback newMessageCallback;

        // The handler to call for a connect/disconnect.
        private DaedalusClient_ConnectCallback connectedCallback;

        // The socket stuff.
        private TcpClient client;
        private NetworkStream ns;

        // The buffer we use for asynchronous reads.
        private byte[] readBuffer;

        private string partialMsg = "";

        // The server port.
        private int port;

        // The server IP address.
        private IPAddress host;

        // ReadCallback and WriteCallback get called asynchronously, so lets
        // employ a simple lock and a flag to make sure we don't try to shut
        // down as data is being processed.
        private object locker = new object();

        private volatile bool connecting = false;
        public bool Connecting
        {
            get
            {
                return connecting;
            }
        }

        private volatile bool connected = false;
        public bool Connected
        {
            get
            {
                return connected;
            }
        }

        public DaedalusClient(DaedalusClient_ConnectCallback aConnectedCallbackHandler, DaedalusClient_NewMessageCallback aNewMessageCallbackHandler)
        {
            this.connectedCallback = aConnectedCallbackHandler;
            this.newMessageCallback = aNewMessageCallbackHandler;
        }

        // Start up the worker thread to set us up a connection.
        public void Connect(IPAddress aHost, int aPort)
        {
            lock (this.locker)
            {
                if (this.connecting)
                    return;

                if (this.connected)
                    return;

                this.connecting = true;

                this.port = aPort;
                this.host = aHost;

                this.readBuffer = new byte[1024];

                // Set up a TCP listener and start listening.
                this.client = new TcpClient();
                // It's ok if this throws an exception, we will catch it up a layer.
                this.client.BeginConnect(this.host, this.port, NewTCPConnectionEvent, null);
            }
        }

        // Tell the worker thread to shut down.
        public void Disconnect()
        {
            lock (this.locker)
            {
                if (!this.connected && !this.connecting)
                    return;

                this.client.Close();

                this.connected = false;
                this.connecting = false;

                this.connectedCallback();
            }
        }

        // TcpListener callback handler.
        private void NewTCPConnectionEvent(IAsyncResult ar)
        {
            lock (this.locker)
            {
                try
                {
                    this.client.EndConnect(ar);
                }
                catch (Exception)
                {
                    // Did not connect successfully.
                    Disconnect();
                    return;
                }

                this.client.NoDelay = true;

                this.connected = true;
                this.connecting = false;

                this.ns = this.client.GetStream();

                try
                {
                    this.ns.BeginRead(this.readBuffer, 0, this.readBuffer.Length, ReadCallbackHandler, null);
                }
                catch (Exception)
                {
                    // Disconnected.
                    Disconnect();
                    return;
                }

                this.connectedCallback();
            }
        }

        // New data has been read.
        private void ReadCallbackHandler(IAsyncResult ar)
        {
            int bytesRead = 0;
            string msg;

            lock (this.locker)
            {
                if (!this.connected)
                    return;

                try
                {
                    bytesRead = this.ns.EndRead(ar);
                }
                catch (Exception)
                {
                    // Disconnected.
                    Disconnect();
                    return;
                }

                if (bytesRead == 0)
                {
                    // Disconnected.
                    Disconnect();
                    return;
                }

                // Copy the data into a local buffer.
                msg = Encoding.ASCII.GetString(this.readBuffer, 0, bytesRead);

                // Split on carriage return and linefeed.
                string[] msgs = msg.Split(new string[] { "\r\n" }, StringSplitOptions.RemoveEmptyEntries);

                if (msgs.Length == 1 && !msg.EndsWith("\r\n"))
                {
                    // Only a partial command in this data.
                    partialMsg += msgs[0];
                }
                else
                {
                    if (partialMsg.Length > 0)
                    {
                        // Place the partial message onto the front of this list of
                        // messages.
                        msgs[0] = partialMsg + msgs[0];
                        partialMsg = "";
                    }

                    int commandCount = msgs.Length;
                    if (!msg.EndsWith("\r\n"))
                    {
                        // The final message is incomplete--wait for the rest.
                        partialMsg += msgs[msgs.Length - 1];
                        commandCount--;
                    }

                    // Pass back each message.
                    for (int i = 0; i < commandCount; i++)
                    {
                        this.newMessageCallback(msgs[i]);
                    }
                }

                // Start a new read.
                try
                {
                    this.ns.BeginRead(this.readBuffer, 0, this.readBuffer.Length, this.ReadCallbackHandler, null);
                }
                catch
                {
                    // Failed to start read!
                    Disconnect();
                    return;
                }
            }
        }

        public void SendMessage(string msg)
        {
            lock (this.locker)
            {
                if (!this.connected)
                    return;

                byte[] data = Encoding.ASCII.GetBytes(msg + "\r\n");

                try
                {
                    this.ns.Write(data, 0, data.Length);

                    this.ns.Flush();
                }
                catch
                {
                    // Disconnected.
                    Disconnect();
                    return;
                }
            }
        }
    }
}