/* $Id: NetworkServerEventHub.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The NetworkServerEventHub class manages the passing of events
 * between the client threads and the server.
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
using System.Text;
using System.Threading;

namespace DaedalusGameManager
{
    public class NetworkServerEventHub
    {
        private Queue<NetworkServerEventInfo> eventQ;

        private Thread eventThread;

        private object eventLock = new object();

        private volatile bool stopEventThread;

        private NetworkServerEventHandlerType eventCallback;

        public NetworkServerEventHub(NetworkServerEventHandlerType aHandler)
        {
            this.eventCallback = aHandler;

            this.eventQ = new Queue<NetworkServerEventInfo>();
            this.eventThread = new Thread(EventThreadCode);
            this.stopEventThread = false;

            this.eventThread.Start();
            while (!this.eventThread.IsAlive)
                ;
        }

        private void EventThreadCode()
        {
            while (true)
            {
                lock (this.eventLock)
                {
                    while (!this.stopEventThread && this.eventQ.Count == 0)
                        Monitor.Wait(this.eventLock);

                    if (this.stopEventThread)
                        break;
                }

                // Process each new event
                while (true)
                {
                    NetworkServerEventInfo info = default(NetworkServerEventInfo);
                    bool newInfo = false;

                    lock (this.eventLock)
                    {
                        if (this.eventQ.Count <= 0)
                            break;

                        info = this.eventQ.Dequeue();

                        newInfo = true;
                    }

                    if (newInfo)
                        this.eventCallback(info);
                }
            }
        }

        public void Stop()
        {
            lock (this.eventLock)
            {
                this.stopEventThread = true;
                Monitor.Pulse(this.eventLock);
            }

            this.eventThread.Join(0);
        }

        private void QueueEvent(NetworkServerEventInfo info)
        {
            lock (this.eventLock)
            {
                this.eventQ.Enqueue(info);
                Monitor.Pulse(this.eventLock);
            }
        }

        public void TriggerEvent_ClientConnected(NetworkServerClientConnection connection)
        {
            NetworkServerEventInfo info = new NetworkServerEventInfo(NetworkServerEventInfoCode.ClientConnected, connection.Id, "Client " + connection.Id + " (" + connection.RemoteEndPoint.ToString() + ") has connected.");
            QueueEvent(info);
        }

        public void TriggerEvent_ClientDisconnected(NetworkServerClientConnection connection)
        {
            NetworkServerEventInfo info = new NetworkServerEventInfo(NetworkServerEventInfoCode.ClientDisconnected, "Client " + connection.Id + " has disconnected.");
            QueueEvent(info);
        }

        public void TriggerEvent_ServerStopped()
        {
            NetworkServerEventInfo info = new NetworkServerEventInfo(NetworkServerEventInfoCode.ServerStopped, "NetworkServer stopped.");
            QueueEvent(info);
        }

        public void TriggerEvent_LogMessage(NetworkServerClientConnection connection, string msg)
        {
            NetworkServerEventInfo info = new NetworkServerEventInfo(NetworkServerEventInfoCode.LogMessage, connection.Id, msg);
            QueueEvent(info);
        }

        public void TriggerEvent_LogMessage(string msg)
        {
            NetworkServerEventInfo info = new NetworkServerEventInfo(NetworkServerEventInfoCode.LogMessage, msg);
            QueueEvent(info);
        }

        public void TriggerEvent_NewData(NetworkServerClientConnection connection, string msg)
        {
            NetworkServerEventInfo info = new NetworkServerEventInfo(NetworkServerEventInfoCode.NewData, connection.Id, msg);
            QueueEvent(info);
        }
    }
}