/* $Id: GameMessage.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The GameMessage abstract class has concrete subclasses that each
 * implement GameMessage for one of the five message types (Move,
 * YourPlayerNumber, YourTurn, BoardState, and GameOver). The classes can be
 * constructed either with specific parameters or with a single string, which is
 * parsed automatically.  Instances of GameMessage subclasses are implicitly
 * converted to strings on demand.
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
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace DaedalusGameProtocol
{
    public abstract partial class GameMessage
    {
        // Implicit string conversion.
        public static implicit operator string(GameMessage msg)
        {
            return msg.ToString();
        }

        // The data contained in this message.
        protected string data;

        public override string ToString()
        {
            return this.data;
        }

        // Parse the contents of the message by removing the brackets.
        public static string GetMessageData(string msg)
        {
            int i, j;
            try
            {
                i = msg.IndexOf("{");
                j = msg.LastIndexOf("}");
            }
            catch
            {
                return "";
            }

            return msg.Substring(i + 1, j - i - 1);
        }

        // If the message represents a valid GameMessage, return its derived
        // type.
        protected static Type GetMessageType(string msg)
        {
            Type msgType;

            msg = Regex.Replace(msg, "\\s", "");

            try
            {
                string msgTypeString = msg.Substring(0, msg.IndexOf("{"));
                msgType = typeof(GameMessage).GetNestedType(msgTypeString);
            }
            catch
            {
                return null;
            }

            return msgType;
        }

        // Determine if the message contains any valid GameMessage type.
        private static bool IsType(string msg, Type theType)
        {
            Type msgType = GetMessageType(msg);
            return (msgType == theType);
        }

        // Determine if the message is a GameMessage of type Move.
        public static bool IsMove(string msg)
        {
            return IsType(msg, typeof(GameMessage.Move));
        }

        // Determine if the message is a GameMessage of type YourTurn.
        public static bool IsYourTurn(string msg)
        {
            return IsType(msg, typeof(GameMessage.YourTurn));
        }

        // Determine if the message is a GameMessage of type YourPlayerNumber.
        public static bool IsYourPlayerNumber(string msg)
        {
            return IsType(msg, typeof(GameMessage.YourPlayerNumber));
        }

        // Determine if the message is a GameMessage of type Version.
        public static bool IsVersion(string msg)
        {
            return IsType(msg, typeof(GameMessage.Version));
        }

        // Determine if the message is a GameMessage of type BoardState.
        public static bool IsBoardState(string msg)
        {
            return IsType(msg, typeof(GameMessage.BoardState));
        }

        // Determine if the message is a GameMessage of type GameOver.
        public static bool IsGameOver(string msg)
        {
            return IsType(msg, typeof(GameMessage.GameOver));
        }

        // Determine if the message is a GameMessage of type Chat.
        public static bool IsChat(string msg)
        {
            return IsType(msg, typeof(GameMessage.Chat));
        }

        // Determine if the message is a GameMessage of type Control.
        public static bool IsControl(string msg)
        {
            return IsType(msg, typeof(GameMessage.Control));
        }

        // Remove all whitespace characters from a message string.
        public string RemoveAllWhiteSpace(string msg)
        {
            return Regex.Replace(msg, "\\s", "");
        }

        // Placeholder.
        public abstract class Move : GameMessage
        {
        }

        // Placeholder.
        public abstract class Control : GameMessage
        {
        }

        // Placeholder.
        public abstract class BoardState : GameMessage
        {
            public abstract GameBoard Board
            {
                get;
            }
        }
    }
}