/* $Id: GameMessage.GameOver.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Signals to the player that the game is over, and why.
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
        public class GameOver : GameMessage
        {
            protected GameOverCondition condition;

            public GameOverCondition Condition
            {
                get
                {
                    return condition;
                }
            }

            // Constructor instantiates a GameMessage of type GameOver with
            // the specified GameOverCondition.
            public GameOver(GameOverCondition aCondition)
            {
                this.condition = aCondition;
                this.data = string.Format("{0}{{{1}}}", typeof(GameMessage.GameOver).Name.ToString(), this.condition.ToString());
            }

            // Constructor instantiates a GameMessage of type GameOver and sets the
            // GameOverCondition with the data contained in the specified message string.
            public GameOver(string msg)
            {
                if (!GameMessage.IsGameOver(msg))
                    throw new Exception("This is not a GameOver message.");

                // Remove all whitespace characters from the message string.
                string msgString = RemoveAllWhiteSpace(msg);

                this.data = msgString;
                string conditionString = GameMessage.GetMessageData(msgString);

                this.condition = (GameOverCondition)Enum.Parse(typeof(GameOverCondition), conditionString);
            }
        }
    }
}