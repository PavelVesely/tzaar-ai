/* $Id: NetworkServerEventInfo.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The NetworkServerEventInfo class is used to encapsulate
 * information about interactions between the NetworkServerEventHub and the
 * NetworkServer.
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

namespace DaedalusGameManager
{
    // Delegate used by the client to notify the server of events.
    public delegate void NetworkServerEventHandlerType(NetworkServerEventInfo info);

    // A code for each possible event.
    public enum NetworkServerEventInfoCode
    {
        None,
        NewData,
        ServerStopped,
        ClientConnected,
        ClientDisconnected,
        LogMessage,
    }

    // A structure to encapsulate event info.
    public struct NetworkServerEventInfo
    {
        // This code specifies the event.
        private NetworkServerEventInfoCode code;
        public NetworkServerEventInfoCode Code
        {
            get
            {
                return code;
            }
        }

        // The Id of the client that generated this event.
        private int clientId;
        public int ClientId
        {
            get
            {
                return clientId;
            }
        }

        // An optional message.
        private string message;
        public string Message
        {
            get
            {
                return message;
            }
        }

        // An event with specified code.
        public NetworkServerEventInfo(NetworkServerEventInfoCode aCode)
        {
            this.code = aCode;
            this.clientId = -1;
            this.message = null;
        }

        // An event with specified code and clientId.
        public NetworkServerEventInfo(NetworkServerEventInfoCode aCode, int aClientId)
        {
            this.code = aCode;
            this.clientId = aClientId;
            this.message = null;
        }

        // An event with specified code and message.
        public NetworkServerEventInfo(NetworkServerEventInfoCode aCode, string aMessage)
        {
            this.code = aCode;
            this.clientId = -1;
            this.message = aMessage;
        }

        // An event with specified code, clientId, and message.
        public NetworkServerEventInfo(NetworkServerEventInfoCode aCode, int aClientId, string aMessage)
        {
            this.code = aCode;
            this.clientId = aClientId;
            this.message = aMessage;
        }
    }
}