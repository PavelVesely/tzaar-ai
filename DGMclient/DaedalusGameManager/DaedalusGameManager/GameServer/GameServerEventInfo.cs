/* $Id: GameServerEventInfo.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The GameServerEventInfo class is used to encapsulate information
 * about interactions between the DaedalusGameManagerForm and the GameServer.
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
using DaedalusGameProtocol;

namespace DaedalusGameManager
{
    // Delegate used by the client to notify the server of events.
    public delegate void GameServerEventHandlerType(GameServerEventInfo info);

    // A code for each possible event.
    public enum GameServerEventInfoCode
    {
        ServerStopped,
        ClientConnected,
        StateUpdated,
        LogMessage,
    }

    // A structure to encapsulate event info
    public struct GameServerEventInfo
    {
        // This code specifies the event.
        private GameServerEventInfoCode code;
        public GameServerEventInfoCode Code
        {
            get
            {
                return code;
            }
        }

        // The Id of the client that generated this event.
        private GameState state;
        public GameState State
        {
            get
            {
                return state;
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
        public GameServerEventInfo(GameServerEventInfoCode aCode)
        {
            this.code = aCode;
            this.state = null;
            this.message = null;
        }

        // An event with specified code and clientId.
        public GameServerEventInfo(GameServerEventInfoCode aCode, GameState aState)
        {
            this.code = aCode;
            this.state = aState;
            this.message = null;
        }

        // An event with specified code and message.
        public GameServerEventInfo(GameServerEventInfoCode aCode, string aMessage)
        {
            this.code = aCode;
            this.state = null;
            this.message = aMessage;
        }

        // An event with specified code, clientId, and message.
        public GameServerEventInfo(GameServerEventInfoCode aCode, GameState aState, string aMessage)
        {
            this.code = aCode;
            this.state = aState;
            this.message = aMessage;
        }
    }
}