/* $Id: NetworkServerClientConnection.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Each instance of the NetworkServerClientConnection ADT handles
 * the TCP interactions with a particular connected client.
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
using System.ComponentModel;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace DaedalusGameManager
{
    public class NetworkServerClientConnection
    {
        private const int readBufferLen = 4096;  // bytes

        // The buffer we use for asynchronous reads.
        private byte[] readBuffer;

        private string partialMsg = "";

        // The socket stuff.
        private TcpClient actualTcpClient;

        private volatile bool stopped;

        // The event handler for callbacks to the server ADT.
        private NetworkServerClientEventHandlerType clientEventHandler;

        // This client's Id number (used by the server).
        private int id;
        public int Id
        {
            get
            {
                return id;
            }
            set
            {
                id = value;
            }
        }

        // The info for the remote endpoint of this connection.
        public EndPoint RemoteEndPoint
        {
            get
            {
                // We look at the TcpClient's underlying network Client's remote
                // endpoint to get the IP address of the remote host.
                return actualTcpClient.Client.RemoteEndPoint;
            }
        }

        // The client connection ADT is created around the specified TcpClient.
        public NetworkServerClientConnection(TcpClient client, NetworkServerClientEventHandlerType aHandler)
        {
            this.stopped = false;

            this.actualTcpClient = client;
            this.clientEventHandler = aHandler;

            // Don't delay transmission waiting for bigger data loads; send it
            // right away!
            this.actualTcpClient.NoDelay = true;

            // The read buffer for new messages from the client.
            this.readBuffer = new byte[readBufferLen];

            BeginAsynchronousRead();
        }

        // Stop the connection.
        public void Stop()
        {
            if (this.stopped)
                // The connection is already stopped; do nothing.
                return;

            this.stopped = true;

            // Shutdown the client.
            this.actualTcpClient.Close();
        }

        private void BeginAsynchronousRead()
        {
            // Start an asynchronous read.
            try
            {
                NetworkStream ns = this.actualTcpClient.GetStream();
                ns.BeginRead(this.readBuffer, 0, this.readBuffer.Length, this.AsynchronousRead, null);
            }
            catch
            {
                // Something exploded.  Stop the connection.
                Stop();
                TriggerEvent_Disconnected();
                return;
            }
        }

        // Called when new data is available to read.
        private void AsynchronousRead(IAsyncResult ar)
        {
            if (this.stopped)
                // The connection is stopped; do nothing.
                return;

            string msg;

            int bytesRead = 0;

            // Tell it to put the data into the read buffer.
            try
            {
                NetworkStream ns = this.actualTcpClient.GetStream();
                bytesRead = ns.EndRead(ar);
            }
            catch
            {
                // Something went wrong.  Stop the connection.
                Stop();
                TriggerEvent_Disconnected();
                return;
            }

            if (bytesRead == 0)
            {
                // A read with no data?  Stop the connection.
                Stop();
                TriggerEvent_Disconnected();
                return;
            }

            // Copy the message out of the read buffer so we can reuse it.
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
                    TriggerEvent_NewData(msgs[i]);
                }
            }

            BeginAsynchronousRead();
        }

        // Write a message to the client.
        public void WriteLine(string msg)
        {
            if (this.stopped)
                // The connection is stopped; do nothing.
                return;

            // Format the data for sending.
            byte[] data = Encoding.ASCII.GetBytes(msg + "\r\n");

            // Send the message.
            try
            {
                NetworkStream ns = this.actualTcpClient.GetStream();
                ns.Write(data, 0, data.Length);
                // Flush the stream to make sure the packet gets sent ASAP.
                ns.Flush();
            }
            catch
            {
                // Sending failed!  Stop the connection.
                Stop();
                TriggerEvent_Disconnected();
                return;
            }
        }

        // Generate a NewData event to pass the server a newly received message.
        private void TriggerEvent_NewData(string msg)
        {
            NetworkServerClientEventInfo info = new NetworkServerClientEventInfo(this, NetworkServerClientEventInfoCode.NewData, msg);
            this.clientEventHandler(info);
        }

        // Generate a Disconnected event to notify the server that the client
        // has disconnected.
        private void TriggerEvent_Disconnected()
        {
            NetworkServerClientEventInfo info = new NetworkServerClientEventInfo(this, NetworkServerClientEventInfoCode.Disconnected);
            this.clientEventHandler(info);
        }

        // Generate a LogMessage event to add a log message.
        private void TriggerEvent_LogMessage(string msg)
        {
            NetworkServerClientEventInfo info = new NetworkServerClientEventInfo(this, NetworkServerClientEventInfoCode.LogMessage, msg);
            this.clientEventHandler(info);
        }
    }
}