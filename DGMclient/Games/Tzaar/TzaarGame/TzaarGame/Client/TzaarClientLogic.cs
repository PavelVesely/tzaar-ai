/* $Id: TzaarClientLogic.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The client logic is the code necessary for the GUI client to
 * interact with the game.
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
using System.Drawing;

using System.Text;
using System.Windows.Forms;
using DaedalusGameProtocol;

namespace TzaarGame
{
    public class TzaarClientLogic : IGameClientLogic
    {
        private TzaarClientPainter painter;
        private TzaarLogic logic;

        private Point fromPoint;

        private GamePlayer playerNumber;
        private PictureBox pictureBox1;
        private GameMessage.Version versionMsg;

        public TzaarClientLogic(PictureBox aPictureBox, GameMessage.Version versionMsg, GamePlayer playerNumber, GameBoard initialBoard)
        {
            this.pictureBox1 = aPictureBox;
            this.versionMsg = versionMsg;
            this.painter = new TzaarClientPainter(this.pictureBox1);
            this.playerNumber = playerNumber;
            this.painter.SetLocalPlayerColor((playerNumber == GamePlayer.One) ? TzaarColor.WHITE : TzaarColor.BLACK);
            this.logic = new TzaarLogic(new TzaarGameState((TzaarBoard)initialBoard));
        }

        public GameMessage.Move GetNewMoveGameMessage(string moveMsg)
        {
            return new TzaarMessage.Move(moveMsg);
        }

        public GameMessage.BoardState GetNewBoardStateGameMessage(string boardMsg)
        {
            return new TzaarMessage.BoardState(boardMsg);
        }

        public GameState GetGameState()
        {
            // Ensure a consistent state when reading, by handing back a copy.
            return this.logic.GetGameState().Copy();
        }

        public GameMessage.Move HandleClick(GamePlayer playerNumber, Point clickLoc)
        {
            TzaarGameState state = (TzaarGameState)this.logic.GetGameState();
            if (state.GameIsOver)
                throw new Exception("Game is over.");

            if (this.logic.GetGameState().GetCurrentPlayerNumber() != playerNumber)
                throw new Exception("Not our turn");

            if (this.painter.IsPassClick(clickLoc))
            {
                if (!state.IsFirstMoveOfTurn)
                {
                    this.painter.ClearSelection();

                    // Make a new 'Pass' move.
                    return new TzaarMessage.Move();
                }
                else
                {
                    return null;
                }
            }

            Point pos = this.painter.PictureBoxCoordsToBoardPosition(clickLoc);

            Stack<TzaarPiece> pieces = state.Board.Query(Convert.ToInt32(pos.X), Convert.ToInt32(pos.Y));
            if (pieces == null)
                // This position isn't even on the board. This should never
                // happen.
                throw new Exception("Click out of bounds.");

            if (pieces.Count == 0)
                // Don't allow selection of empty positions.
                throw new Exception("Empty position.");

            if (!this.painter.APositionIsSelected)
            {
                TzaarColor myColor = (playerNumber == GamePlayer.One) ? TzaarColor.WHITE : TzaarColor.BLACK;

                if (myColor != pieces.Peek().Color)
                    // Don't allow selection of other player's pieces.
                    throw new Exception("Can't select other player's pieces.");

                this.painter.SelectPosition(pos.X, pos.Y);

                this.fromPoint = pos;

                // A position has been selected.
                return null;
            }
            else
            {
                this.painter.ClearSelection();

                if (pos.X == fromPoint.X && pos.Y == fromPoint.Y)
                    // Clicking the position twice un-selects.
                    return null;

                return new TzaarMessage.Move(Convert.ToInt32(fromPoint.X), Convert.ToInt32(fromPoint.Y), Convert.ToInt32(pos.X), Convert.ToInt32(pos.Y));
            }
        }

        public void HandleMove(GamePlayer playerNumber, GameMessage.Move moveMsg)
        {
            this.logic.MakeMove(playerNumber, moveMsg);

            this.logic.GetGameState().AddMove(moveMsg);

            if (playerNumber == this.playerNumber)
            {
                TzaarGameState state = (TzaarGameState)this.logic.GetGameState();

                if (state.GetCurrentPlayerNumber() == playerNumber && !state.IsFirstMoveOfTurn)
                    this.painter.PassButtonEnabled = true;
                else
                    this.painter.PassButtonEnabled = false;
            }
        }

        public void Paint(Graphics g, GameState gameState)
        {
            this.painter.Draw(g, gameState);
        }

        public void EndGame(GamePlayer winner, GameOverCondition condition)
        {
            this.logic.GetGameState().SetGameOver(winner, condition);
        }

        public string GetGameName()
        {
            return "Tzaar";
        }
    }
}