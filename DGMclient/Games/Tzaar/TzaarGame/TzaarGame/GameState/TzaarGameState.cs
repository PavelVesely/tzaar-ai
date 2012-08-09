/* $Id: TzaarGameState.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The game state holds the board state, as well as scoring and
 * turn information.
 *
 * Copyright (c) 2010-2011, Team Daedalus (Mathew Bergt, Jason Buck, Ken Kelley, and
 * Justin Weaver).
 *
 * Distributed under the BSD-new license. For details see the BSD_LICENSE file
 * that should have been included with this distribution. If the source you
 * acquired this distribution from incorrectly removed this file, the license
 * may be viewed at http://www.opensource.org/licenses/bsd-license.php.
 */

using System.Collections.Generic;
using DaedalusGameProtocol;

namespace TzaarGame
{
    public class TzaarGameState : GameState
    {
        public new TzaarBoard Board
        {
            get
            {
                return (TzaarBoard)board;
            }
        }

        // Constructor to initialize a new game state using the specified
        // GameBoard.
        public TzaarGameState(GameBoard aBoard)
            : base(aBoard)
        {
        }

        // Constructor to initialize a new game state using the specified
        // GameBoard and totalMoveCount.
        public TzaarGameState(GameBoard aBoard, int totalMoveCount)
            : base(aBoard)
        {
            this.totalMoveCount = totalMoveCount;
        }

        // Get the color of the player designated to move this turn.
        public override GamePlayer GetCurrentPlayerNumber()
        {
            int index = this.totalMoveCount % 4;
            return (index == 0 || index == 3) ? GamePlayer.One : GamePlayer.Two;
        }

        // There are two moves for each turn, so return the current move number.
        public bool IsFirstMoveOfTurn
        {
            get
            {
                return ((this.totalMoveCount == 0) || ((this.totalMoveCount) % 2 != 0));
            }
        }

        // Return a full copy of the current game state.
        public override GameState Copy()
        {
            TzaarGameState state = new TzaarGameState((TzaarBoard)this.Board.Copy());

            state.totalMoveCount = this.totalMoveCount;
            state.gameIsOver = this.gameIsOver;
            state.gameOverCondition = GameOverCondition.None;
            state.winningPlayerNumber = this.winningPlayerNumber;
            state.history = new Stack<string>(this.history);

            return state;
        }
    }
}