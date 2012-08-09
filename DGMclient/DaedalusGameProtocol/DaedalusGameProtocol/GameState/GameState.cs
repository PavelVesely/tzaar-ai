/* $Id: GameState.cs 14 2011-12-22 00:41:30Z piranther $
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

namespace DaedalusGameProtocol
{
    public abstract class GameState
    {
        // Constructor to initialize a new game state using the specified
        // GameBoard.
        public GameState(GameBoard aBoard)
        {
            this.board = aBoard;
            this.gameIsOver = false;
            this.totalMoveCount = 0;
            this.gameOverCondition = GameOverCondition.None;
            this.winningPlayerNumber = GamePlayer.None;
            this.history = new Stack<string>();
        }

        // The message log leading up to this game state.
        protected Stack<string> history;
        public Stack<string> History
        {
            get
            {
                return this.history;
            }
        }

        // The GameBoard for this game state.
        protected GameBoard board;
        public GameBoard Board
        {
            get
            {
                return this.board;
            }
        }

        // The total number of moves in the current game state.
        protected volatile int totalMoveCount;
        public int TotalMoveCount
        {
            get
            {
                return this.totalMoveCount;
            }
        }

        protected volatile bool gameIsOver;
        public bool GameIsOver
        {
            get
            {
                return this.gameIsOver;
            }
        }

        protected volatile GameOverCondition gameOverCondition;
        public GameOverCondition GameOverCondition
        {
            get
            {
                return this.gameOverCondition;
            }
        }

        // If the game is over, and it wasn't a draw, who won?
        protected volatile GamePlayer winningPlayerNumber;
        public GamePlayer WinningPlayerNumber
        {
            get
            {
                return this.winningPlayerNumber;
            }
        }

        public void SetGameOver(GamePlayer winner, GameOverCondition condition)
        {
            this.winningPlayerNumber = winner;
            this.gameOverCondition = condition;
            this.gameIsOver = true;
        }

        // Step to the next move.
        public void AddMove(string move)
        {
            this.history.Push(move);
            this.totalMoveCount++;
        }

        // Get the player number of the player designated to move this turn.
        public abstract GamePlayer GetCurrentPlayerNumber();

        // Return a full copy of the current game state.
        public abstract GameState Copy();
    }
}