/* $Id: TzaarBoard.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Holds a snapshot of a board state, i.e. which positions have
 * pieces on them, how many pieces, and which type and color.
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

using DaedalusGameProtocol;

namespace TzaarGame
{
    public partial class TzaarBoard : GameBoard
    {
        // Data structure to represent the contents of each position on the game
        // board.
        private TzaarBoardPosition[][] board;

        // The various counts for different game pieces.
        private int blackTzaarCount;
        public int BlackTzaarCount
        {
            get
            {
                return blackTzaarCount;
            }
        }

        private int blackTzarraCount;
        public int BlackTzarraCount
        {
            get
            {
                return blackTzarraCount;
            }
        }

        private int blackTottCount;
        public int BlackTottCount
        {
            get
            {
                return blackTottCount;
            }
        }

        private int whiteTzaarCount;
        public int WhiteTzaarCount
        {
            get
            {
                return whiteTzaarCount;
            }
        }

        private int whiteTzarraCount;
        public int WhiteTzarraCount
        {
            get
            {
                return whiteTzarraCount;
            }
        }

        private int whiteTottCount;
        public int WhiteTottCount
        {
            get
            {
                return whiteTottCount;
            }
        }

        public TzaarBoard()
            : this(false)
        {
        }

        public TzaarBoard(bool empty)
        {
            this.board = new TzaarBoardPosition[9][];

            this.board[0] = new TzaarBoardPosition[5];
            this.board[1] = new TzaarBoardPosition[6];
            this.board[2] = new TzaarBoardPosition[7];
            this.board[3] = new TzaarBoardPosition[8];
            this.board[4] = new TzaarBoardPosition[8];
            this.board[5] = new TzaarBoardPosition[8];
            this.board[6] = new TzaarBoardPosition[7];
            this.board[7] = new TzaarBoardPosition[6];
            this.board[8] = new TzaarBoardPosition[5];

            // Create new Node objects in the board array, named in (col,row)
            // format.
            for (int i = 0; i < board.Length; i++)
                for (int j = 0; j < board[i].Length; j++)
                    board[i][j] = new TzaarBoardPosition();

            // Initialize the piece counters.
            this.whiteTzaarCount = 0;
            this.whiteTzarraCount = 0;
            this.whiteTottCount = 0;
            this.blackTzaarCount = 0;
            this.blackTzarraCount = 0;
            this.blackTottCount = 0;

            if (!empty)
                RandomizeBoardState(this);
        }

        // Return true if the specified board position exists.
        public bool IsValidPosition(int col, int row)
        {
            if (col < 0 || col > 8 || row < 0 || row > this.board[col].Length - 1)
                return false;

            if (this.board[col][row] == null)
                return false;

            return true;
        }

        // Scan the board and update the counts for each piece still remaining.
        private static void UpdatePieceCount(TzaarBoard board)
        {
            // The entire board is scanned for the update, so destroy the old
            // count values.
            board.blackTzaarCount = 0;
            board.blackTzarraCount = 0;
            board.blackTottCount = 0;

            board.whiteTzaarCount = 0;
            board.whiteTzarraCount = 0;
            board.whiteTottCount = 0;

            // Scan each space of the board to update.
            for (int i = 0; i < 9; i++)
                for (int j = 0; j < 9; j++)
                {
                    // If the position is not valid, we have passed the end of
                    // the column. Move to the next column.
                    if (!board.IsValidPosition(i, j))
                        break;

                    // Otherwise, find the color and number of pieces at the
                    // current position and update accordingly.
                    Stack<TzaarPiece> S = board.board[i][j].Query();
                    if (S.Count == 0)
                        continue;

                    TzaarPiece topPiece = S.Peek();
                    if (topPiece.Color == TzaarColor.WHITE)
                    {
                        if (topPiece.GetType() == typeof(TzaarPiece.Tzaar))
                            board.whiteTzaarCount++;
                        else if (topPiece.GetType() == typeof(TzaarPiece.Tzarra))
                            board.whiteTzarraCount++;
                        else
                            board.whiteTottCount++;
                    }
                    else
                    {
                        if (topPiece.GetType() == typeof(TzaarPiece.Tzaar))
                            board.blackTzaarCount++;
                        else if (topPiece.GetType() == typeof(TzaarPiece.Tzarra))
                            board.blackTzarraCount++;
                        else
                            board.blackTottCount++;
                    }
                }
        }

        // Add a stack of pieces to the specified position.
        public void Add(Stack<TzaarPiece> somePieces, int col, int row)
        {
            if (!this.IsValidPosition(col, row))
                return;

            Type t;
            Stack<TzaarPiece> s = board[col][row].Query();
            if (s.Count > 0) {
              t = s.Peek().GetType();
              if (s.Peek().Color == TzaarColor.WHITE)
              {
                  if (t == typeof(TzaarPiece.Tzaar))
                      this.whiteTzaarCount--;
                  else if (t == typeof(TzaarPiece.Tzarra))
                      this.whiteTzarraCount--;
                  else
                      this.whiteTottCount--;
              }
              else
              {
                  if (t == typeof(TzaarPiece.Tzaar))
                      this.blackTzaarCount--;
                  else if (t == typeof(TzaarPiece.Tzarra))
                      this.blackTzarraCount--;
                  else
                      this.blackTottCount--;
              }
            }

            board[col][row].Add(somePieces);

            // Count the pieces we just added.
            //foreach (TzaarPiece p in somePieces)
            if (somePieces.Count > 0) {
                TzaarPiece p = somePieces.Peek();
                t = p.GetType();
                if (p.Color == TzaarColor.WHITE)
                {
                    if (t == typeof(TzaarPiece.Tzaar))
                        this.whiteTzaarCount++;
                    else if (t == typeof(TzaarPiece.Tzarra))
                        this.whiteTzarraCount++;
                    else
                        this.whiteTottCount++;
                }
                else
                {
                    if (t == typeof(TzaarPiece.Tzaar))
                        this.blackTzaarCount++;
                    else if (t == typeof(TzaarPiece.Tzarra))
                        this.blackTzarraCount++;
                    else
                        this.blackTottCount++;
                }
            }
        }

        // Add a piece to the specified position.
        public void Add(TzaarPiece aPiece, int col, int row)
        {
            if (!this.IsValidPosition(col, row))
                return;
            
            Type t;
            Stack<TzaarPiece> s = board[col][row].Query();
            if (s.Count > 0) {
              t = s.Peek().GetType();
              if (s.Peek().Color == TzaarColor.WHITE)
              {
                  if (t == typeof(TzaarPiece.Tzaar))
                      this.whiteTzaarCount--;
                  else if (t == typeof(TzaarPiece.Tzarra))
                      this.whiteTzarraCount--;
                  else
                      this.whiteTottCount--;
              }
              else
              {
                  if (t == typeof(TzaarPiece.Tzaar))
                      this.blackTzaarCount--;
                  else if (t == typeof(TzaarPiece.Tzarra))
                      this.blackTzarraCount--;
                  else
                      this.blackTottCount--;
              }
            }

            board[col][row].Add(aPiece);

            // Count the piece we just added.
            t = aPiece.GetType();
            if (aPiece.Color == TzaarColor.WHITE)
            {
                if (t == typeof(TzaarPiece.Tzaar))
                    this.whiteTzaarCount++;
                else if (t == typeof(TzaarPiece.Tzarra))
                    this.whiteTzarraCount++;
                else
                    this.whiteTottCount++;
            }
            else
            {
                if (t == typeof(TzaarPiece.Tzaar))
                    this.blackTzaarCount++;
                else if (t == typeof(TzaarPiece.Tzarra))
                    this.blackTzarraCount++;
                else
                    this.blackTottCount++;
            }
        }

        // Remove and return the pieces at the specified position.
        public Stack<TzaarPiece> Take(int col, int row)
        {
            if (!this.IsValidPosition(col, row))
                return null;

            Stack<TzaarPiece> S = board[col][row].Take();

            // Count the pieces we just removed.
            //foreach (TzaarPiece p in S)
            if (S.Count > 0) {
                TzaarPiece p = S.Peek();
                Type t = p.GetType();
                if (p.Color == TzaarColor.WHITE)
                {
                    if (t == typeof(TzaarPiece.Tzaar))
                        this.whiteTzaarCount--;
                    else if (t == typeof(TzaarPiece.Tzarra))
                        this.whiteTzarraCount--;
                    else
                        this.whiteTottCount--;
                }
                else
                {
                    if (t == typeof(TzaarPiece.Tzaar))
                        this.blackTzaarCount--;
                    else if (t == typeof(TzaarPiece.Tzarra))
                        this.blackTzarraCount--;
                    else
                        this.blackTottCount--;
                }
            }

            return S;
        }

        // Return the contents of the specified position.
        public Stack<TzaarPiece> Query(int col, int row)
        {
            if (!this.IsValidPosition(col, row))
                return null;

            return board[col][row].Query();
        }

        // Return a full copy.
        public override GameBoard Copy()
        {
            TzaarBoard b = new TzaarBoard(true);

            for (int i = 0; i < this.board.Length; i++)
            {
                TzaarBoardPosition[] col = this.board[i];
                for (int j = 0; j < col.Length; j++)
                {
                    TzaarBoardPosition n = col[j];

                    if (n == null)
                        continue;

                    b.Add(n.Query(), i, j);
                }
            }
            return b;
        }

        // Randomize the board state.
        private static void RandomizeBoardState(TzaarBoard board)
        {
            // Store the counts in a 2D array for easy access.
            int[][] actual = new int[2][];
            actual[0] = new int[] { 6, 9, 15 };
            actual[1] = new int[] { 6, 9, 15 };

            // Keep track of the number of pieces placed so far.
            int[,] pieces = new int[2, 3];

            // Column lengths of the board.
            int[] colLengths = new int[] { 5, 6, 7, 8, 8, 8, 7, 6, 5 };

            Random rand = new Random();
            int player, type;
            bool valid = false;
            TzaarColor color;

            // Now cycle through each space of the board and choose a random
            // piece to place.
            for (int i = 0; i < colLengths.Length; i++)
            {
                for (int j = 0; j < colLengths[i]; j++)
                {
                    // While a valid piece has not been chosen...
                    while (!valid)
                    {
                        // Randomly choose 0 - black or 1 - white.
                        player = rand.Next(0, 2);
                        // Randomly choose between 0 and 2. This will be one of
                        // the three types of pieces.
                        type = rand.Next(0, 3);
                        // Check that this particular color and type can still
                        // be placed.
                        if (pieces[player, type] < actual[player][type])
                        {
                            Stack<TzaarPiece> s = new Stack<TzaarPiece>();
                            if (player == 0)
                                color = TzaarColor.BLACK;
                            else
                                color = TzaarColor.WHITE;

                            switch (type)
                            {
                                case 0:
                                    s.Push(new TzaarPiece.Tzaar(color));
                                    break;
                                case 1:
                                    s.Push(new TzaarPiece.Tzarra(color));
                                    break;
                                case 2:
                                    s.Push(new TzaarPiece.Tott(color));
                                    break;
                            }
                            board.Add(s, i, j);
                            valid = true;
                            pieces[player, type]++;
                        }
                    }
                    valid = false;
                }
            }
        }
    }
}