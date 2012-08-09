/* $Id: GameMessage.Chat.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Chat messages allow us to implement a chat system between the
 * clients. Their contents should never impact the game being played.  The
 * server should simply facilitate their delivery between clients.
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
using System.Text.RegularExpressions;

namespace DaedalusGameProtocol
{
    public abstract partial class GameMessage
    {
        public class Chat : GameMessage
        {
            public string Message
            {
                get
                {
                    return GameMessage.GetMessageData(this);
                }
            }

            public Chat(string msg)
            {
                msg = Regex.Replace(msg, "\r", "");
                msg = Regex.Replace(msg, "\n", "");
                this.data = typeof(GameMessage.YourPlayerNumber).Name.ToString() + "{" + msg + "}";
            }
        }
    }
}