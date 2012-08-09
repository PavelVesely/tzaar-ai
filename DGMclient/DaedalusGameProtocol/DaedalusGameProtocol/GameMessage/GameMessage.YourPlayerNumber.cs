/* $Id: GameMessage.YourPlayerNumber.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Signals to the player which player number is theirs.
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

namespace DaedalusGameProtocol
{
    public abstract partial class GameMessage
    {
        public class YourPlayerNumber : GameMessage
        {
            // The player's number.
            protected GamePlayer playerNumber;
            public GamePlayer PlayerNumber
            {
                get
                {
                    return playerNumber;
                }
            }

            // A constructor that takes a player number.
            public YourPlayerNumber(GamePlayer aPlayer)
            {
                this.data = typeof(GameMessage.YourPlayerNumber).Name.ToString() + "{" + aPlayer.ToString() + "}";

                this.playerNumber = aPlayer;
            }

            // A constructor that parses a message.
            public YourPlayerNumber(string msg)
            {
                if (!GameMessage.IsYourPlayerNumber(msg))
                    throw new Exception("This is not a YourPlayerNumber message.");

                // Remove all whitespace characters from the message string.
                string msgString = RemoveAllWhiteSpace(msg);

                this.data = msgString;

                string playerNumberString = GameMessage.GetMessageData(msgString);

                this.playerNumber = (GamePlayer)Enum.Parse(typeof(GamePlayer), playerNumberString);
            }
        }
    }
}