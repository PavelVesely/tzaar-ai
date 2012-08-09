/* $Id: TzaarMessage.Move.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Signals to the player the last move of their opponents. Also
 * used by the player to deliver move (and pass) commands to the server. A pass
 * is just a move with no data.
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
using DaedalusGameProtocol;

namespace TzaarGame
{
    public abstract partial class TzaarMessage : GameMessage
    {
        public new class Move : GameMessage.Move
        {
            // The origin column of the piece(s) to be moved.
            private int fromCol;

            public int FromCol
            {
                get
                {
                    return fromCol;
                }
            }

            // The origin row of the piece(s) to be moved.
            private int fromRow;

            public int FromRow
            {
                get
                {
                    return fromRow;
                }
            }

            // The destination column of the piece(s) to be moved.
            private int toCol;

            public int ToCol
            {
                get
                {
                    return toCol;
                }
            }

            // The destination row of the piece(s) to be moved.
            private int toRow;

            public int ToRow
            {
                get
                {
                    return toRow;
                }
            }

            // True if this is a Pass command (as opposed to a Move).
            private bool isPass;

            public bool IsPass
            {
                get
                {
                    return isPass;
                }
            }

            // Constructor instantiates a TzaarMessage of type Move from the
            // specified origin and destination coordinates.
            public Move(int fromCol, int fromRow, int toCol, int toRow)
            {
                // A Capture or Stack Move.
                this.fromCol = fromCol;
                this.fromRow = fromRow;
                this.toCol = toCol;
                this.toRow = toRow;
                this.isPass = false;
                this.data = string.Format("{0}{{{1},{2},{3},{4}}}", typeof(TzaarMessage.Move).Name.ToString(), this.fromCol, this.fromRow, this.toCol, this.toRow);
            }

            // Constructor instantiates a TzaarMessage of type Move specifying
            // no coordinates, which represents a 'Pass' in the game.
            public Move()
            {
                // A Pass Move.
                this.isPass = true;
                this.data = string.Format("{0}{{}}", typeof(TzaarMessage.Move).Name.ToString());
            }

            // Constructor instantiates a TzaarMessage of type Move from the
            // specified message string. Throws an exception if the message
            // string does not contain a valid TzaarMessage of type Move.
            public Move(string msg)
            {
                if (!GameMessage.IsMove(msg))
                    throw new Exception("This is not a Move message.");

                // Remove all whitespace characters from the message string.
                string msgString = RemoveAllWhiteSpace(msg);

                // Strip the packet enclosure.
                string move = TzaarMessage.GetMessageData(msgString);
                if (move.Length != 0)
                {
                    string[] coords = move.Split(',');
                    if (coords.Length != 4)
                        throw new Exception("Move message has invalid format.");

                    this.fromCol = Int32.Parse(coords[0]);
                    this.fromRow = Int32.Parse(coords[1]);
                    this.toCol = Int32.Parse(coords[2]);
                    this.toRow = Int32.Parse(coords[3]);
                    this.isPass = false;
                }
                else
                {
                    // A Pass.
                    this.isPass = true;
                }

                this.data = msgString;
            }
        }
    }
}