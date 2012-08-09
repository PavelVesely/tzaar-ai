/* $Id: TzaarMessage.BoardState.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Signals to the player the initial state of the game board.
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
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using DaedalusGameProtocol;

namespace TzaarGame
{
    public abstract partial class TzaarMessage : GameMessage
    {
        public new class BoardState : GameMessage.BoardState
        {
            private TzaarBoard board;
            public override GameBoard Board
            {
                get
                {
                    return board;
                }
            }

            // Constructor to instantiate a new BoardState based on the
            // specified TzaarBoard object.
            public BoardState(TzaarBoard aBoard)
            {
                this.data = GetStringFromBoardState(aBoard);

                this.board = (TzaarBoard)aBoard.Copy();
            }

            // Constructor parses a message string into a BoardState object.
            // Throws an exception if the message string does not represent a
            // valid BoardState message.
            public BoardState(string msg)
            {
                if (!GameMessage.IsBoardState(msg))
                    throw new Exception("This is not a BoardState message.");

                // Remove all whitespace characters from the message string.
                string msgString = RemoveAllWhiteSpace(msg);

                this.data = msgString;

                // Strip packet wrapper.
                string boardString = GameMessage.GetMessageData(msg);

                boardString = boardString.Substring(1, boardString.Length - 2);

                this.board = GetBoardStateFromString(msgString);
            }

            // Parse a message string into a TzaarBoard object.
            private TzaarBoard GetBoardStateFromString(string msg)
            {
                TzaarBoard board = new TzaarBoard(true);

                // Strip packet wrapper.
                string boardString = TzaarMessage.GetMessageData(msg);

                boardString = boardString.Substring(1, boardString.Length - 2);

                // Split into individual stacks.
                string[] stacks = Regex.Split(boardString, "},{");

                int[] offsets = new int[] { 0, 5, 11, 18, 26, 34, 42, 49, 55 };
                string[] aStack;
                for (int i = 0; i < 9; i++)
                {
                    for (int j = 0; j < 9; j++)
                    {
                        Stack<TzaarPiece> S = board.Query(i, j);
                        if (S == null)
                            break;

                        // Split stack into color (aStack[0]) and individual
                        // pieces.
                        aStack = stacks[j + offsets[i]].Split(',');

                        TzaarColor color = (aStack[0].Equals(TzaarColor.BLACK.ToString())) ? TzaarColor.BLACK : TzaarColor.WHITE;

                        // Add each piece to the board.
                        for (int k = aStack.Length - 1; k >= 0; k--)
                        {
                            String s = aStack[k];
                            if (s.Equals(typeof(TzaarPiece.Tzaar).Name.ToString()))
                                board.Add(new TzaarPiece.Tzaar(color), i, j);
                            else if (s.Equals(typeof(TzaarPiece.Tzarra).Name.ToString()))
                                board.Add(new TzaarPiece.Tzarra(color), i, j);
                            else if (s.Equals(typeof(TzaarPiece.Tott).Name.ToString()))
                                board.Add(new TzaarPiece.Tott(color), i, j);
                        }
                    }
                }

                return board;
            }

            // Returns a string representation of the board state in the format
            // of the BoardState message, defined in the Tzaar Protocol BNF.
            private string GetStringFromBoardState(GameBoard aBoard)
            {
                string stateString = "";

                for (int i = 0; i < 9; i++)
                {
                    for (int j = 0; j < 9; j++)
                    {
                        Stack<TzaarPiece> S = ((TzaarBoard)aBoard).Query(i, j);
                        if (S == null)
                            break;

                        stateString += "{";

                        if (S.Count > 0)
                        {
                            stateString += S.Peek().Color.ToString() + ",";
                            foreach (TzaarPiece P in S)
                                stateString += P.GetType().Name.ToString() + ",";

                            // Remove the last comma.
                            stateString = stateString.Substring(0, stateString.Length - 1);
                        }

                        stateString += "},";
                    }
                }

                // Remove the last comma.
                stateString = stateString.Substring(0, stateString.Length - 1);

                return typeof(GameMessage.BoardState).Name.ToString() + "{" + stateString + "}";
            }
        }
    }
}