/* $Id: TzaarGameInterface.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: This is how the game plugs in to the manager.
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
using System.Windows.Forms;
using DaedalusGameProtocol;

namespace TzaarGame
{
    public class TzaarGameInterface : IGame
    {
        // Return a string that uniquely identifies this game.
        public string GetGameName()
        {
            return "Tzaar";
        }

        // Construct and return a new instance of the game-specific GamePainter.
        public IGamePainter GetNewGamePainter(System.Windows.Forms.PictureBox aPictureBox)
        {
            return new TzaarPainter(aPictureBox);
        }

        public IGameLogic GetNewGameLogic(GameState aState)
        {
            return new TzaarLogic(aState);
        }

        // Construct and return a new instance of the game-specific GameState.
        public GameState GetNewGameState()
        {
            return new TzaarGameState(new TzaarBoard());
        }

        public GameState GetNewGameState(GameBoard aBoard)
        {
            return new TzaarGameState(aBoard);
        }

        // Given a game-specific board object, construct and return a new
        // game-specific BoardState message.
        public GameMessage.BoardState GetNewBoardStateGameMessage(GameBoard aBoard)
        {
            return new TzaarMessage.BoardState((TzaarBoard)aBoard);
        }

        public GameMessage.BoardState GetNewBoardStateGameMessage(string aBoardMsgString)
        {
            return new TzaarMessage.BoardState(aBoardMsgString);
        }
    }
}