/* $Id: NetworkServer.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The NetworkServer class is what the GameServer class relies on
 * to perform all of its actual TCP networking work.
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

namespace DaedalusGameManager
{
    public class NetworkServer
    {
        // The listener is used to monitor for incoming connection requests.
        private TcpListener listener;

        // True when Stop() has already been called.
        private volatile bool shutDown;

        // Individual client connections.
        private NetworkServerClientConnection connection1, connection2;

        private NetworkServerEventHub eventHub;

        // This lock is used to synchronize access to local objects by the two
        // event handlers and the Stop method.
        private object connectionLock = new object();

        public NetworkServer(int aPort, NetworkServerEventHandlerType aHandler)
        {
            this.shutDown = false;

            this.connection1 = null;
            this.connection2 = null;

            this.eventHub = new NetworkServerEventHub(aHandler);

            // Set up a TCP listener.
            this.listener = new TcpListener(new IPEndPoint(IPAddress.Any, aPort));

            // Start the listener.
            this.listener.Start();

            // Begin waiting for a new connection.
            this.listener.BeginAcceptTcpClient(NewTCPConnectionEvent, null);
        }

        public void Stop()
        {
            lock (this.connectionLock)
            {
                if (this.shutDown)
                    // We have already shutdown.  No need to do it again.
                    return;

                this.shutDown = true;

                this.listener.Stop();

                this.eventHub.Stop();

                // Stop existing connections.
                if (this.connection1 != null)
                {
                    this.connection1.Stop();
                    this.connection1 = null;
                }

                if (this.connection2 != null)
                {
                    this.connection2.Stop();
                    this.connection2 = null;
                }
            }
        }

        public void SendMessage(int clientId, string msg)
        {
            NetworkServerClientConnection connection;

            lock (this.connectionLock)
            {
                if (clientId == 1)
                    connection = this.connection1;
                else
                    connection = this.connection2;

                if (connection != null)
                    connection.WriteLine(msg);
            }
        }

        public EndPoint GetClientRemoteEndPoint(int clientId)
        {
            lock (this.connectionLock)
            {
                NetworkServerClientConnection connection;

                if (clientId == 1)
                    connection = this.connection1;
                else
                    connection = this.connection2;

                if (connection != null)
                    return connection.RemoteEndPoint;
                else
                    return null;
            }
        }

        private void EmergencyStop()
        {
            Stop();
            this.eventHub.TriggerEvent_ServerStopped();
        }

        // When a new TcpClient connection event happens, this gets called.
        private void NewTCPConnectionEvent(IAsyncResult ar)
        {
            lock (this.connectionLock)
            {
                // Accept the client connection.
                TcpClient client;
                try
                {
                    client = this.listener.EndAcceptTcpClient(ar);
                }
                catch (Exception ex)
                {
                    // Connection attempt failed!
                    this.eventHub.TriggerEvent_LogMessage("Connection Attempt Failed: " + ex.Message);
                    EmergencyStop();
                    return;
                }

                // Create the new client connection and add it to our list of
                // connections.
                NetworkServerClientConnection connection = new NetworkServerClientConnection(client, ClientEventHandler);

                if (this.connection1 == null)
                {
                    this.connection1 = connection;
                    connection.Id = 1;
                }
                else
                {
                    this.connection2 = connection;
                    connection.Id = 2;
                }

                this.eventHub.TriggerEvent_ClientConnected(connection);

                if (this.connection1 == null || this.connection2 == null)
                {
                    // If we don't have all the connections we need yet, keep
                    // listening.

                    try
                    {
                        // Now wait for the next connection.
                        this.listener.BeginAcceptTcpClient(NewTCPConnectionEvent, null);
                    }
                    catch (Exception ex)
                    {
                        // Error: write a log message, stop the server, and
                        // return.
                        this.eventHub.TriggerEvent_LogMessage("BeginAcceptTcpClient Failed: " + ex.Message);
                        EmergencyStop();
                        return;
                    }
                }
            }
        }

        // Called by the client connections when they have an event to report.
        private void ClientEventHandler(NetworkServerClientEventInfo info)
        {
            lock (this.connectionLock)
            {
                if (info.Code == NetworkServerClientEventInfoCode.NewData)
                {
                    this.eventHub.TriggerEvent_NewData(info.Client, info.Message);
                }
                else if (info.Code == NetworkServerClientEventInfoCode.Disconnected)
                {
                    this.eventHub.TriggerEvent_ClientDisconnected(info.Client);

                    if (info.Client.Id == 1)
                        this.connection1 = null;
                    else
                        this.connection2 = null;
                }
                else if (info.Code == NetworkServerClientEventInfoCode.LogMessage)
                {
                    this.eventHub.TriggerEvent_LogMessage(info.Client, info.Message);
                }
                else
                {
                    // Should never be here!
                    new Exception("Client generated an event with code " + info.Code);
                }
            }
        }
    }
}