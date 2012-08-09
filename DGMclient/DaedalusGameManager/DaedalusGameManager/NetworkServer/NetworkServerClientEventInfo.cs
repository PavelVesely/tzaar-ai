/* $Id: NetworkServerClientEventInfo.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The NetworkServerClientEventInfo class is used to encapsulate
 * information about interactions between the NetworkServer and the
 * NetworkServerClientConnection.
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
    public delegate void NetworkServerClientEventHandlerType(NetworkServerClientEventInfo info);

    // A code for each possible event.
    public enum NetworkServerClientEventInfoCode
    {
        None,
        NewData,
        Disconnected,
        LogMessage,
    }

    // A structure to encapsulate event info.
    public struct NetworkServerClientEventInfo
    {
        // This code specifies the event.
        private NetworkServerClientEventInfoCode code;
        public NetworkServerClientEventInfoCode Code
        {
            get
            {
                return code;
            }
        }

        // The client that generated this event.
        private NetworkServerClientConnection client;
        public NetworkServerClientConnection Client
        {
            get
            {
                return client;
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

        // An event with specified client and code.
        public NetworkServerClientEventInfo(NetworkServerClientConnection aClient, NetworkServerClientEventInfoCode aCode)
        {
            this.code = aCode;
            this.message = null;
            this.client = aClient;
        }

        // An event with specified code and message.
        public NetworkServerClientEventInfo(NetworkServerClientConnection aClient, NetworkServerClientEventInfoCode aCode, string aMessage)
        {
            this.code = aCode;
            this.message = aMessage;
            this.client = aClient;
        }
    }
}