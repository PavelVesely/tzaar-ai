/* $Id: TzaarBoardPosition.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: And TzaarBoard.Position object represents a position on the
 * TzaarBoard.  It contains all necessary information about the state of that
 * position (how many pieces are on it, and what those pieces are).
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

namespace TzaarGame
{
    public class TzaarBoardPosition
    {
        private Stack<TzaarPiece> pieces;

        // Constructor initializes a new stack to hold potential game pieces.
        public TzaarBoardPosition()
        {
            this.pieces = new Stack<TzaarPiece>();
        }

        // Add a game piece to the stack at this position.
        public void Add(TzaarPiece aPiece)
        {
            this.pieces.Push(aPiece);
        }

        // Add multiple game pieces to the stack at this position.
        public void Add(Stack<TzaarPiece> somePieces)
        {
            TzaarPiece[] arr = somePieces.ToArray();
            for (int i = arr.Length - 1; i >= 0; i--)
                this.pieces.Push(arr[i]);
        }

        // Remove and return all game pieces at this position, if any.
        public Stack<TzaarPiece> Take()
        {
            Stack<TzaarPiece> SS = this.pieces;
            this.pieces = new Stack<TzaarPiece>();
            return SS;
        }

        // Returns the pieces contained in this position.
        public Stack<TzaarPiece> Query()
        {
            return this.pieces;
        }
    }
}