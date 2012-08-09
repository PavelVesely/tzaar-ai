/* $Id: IGame.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: This is the interface games used to hook in to Daedalus Game
 * Manager.
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

namespace DaedalusGameProtocol
{
    public interface IGame
    {
        // Return a string that uniquely identifies this game.
        string GetGameName();

        // Construct and return a new instance of the game-specific GamePainter.
        IGamePainter GetNewGamePainter(PictureBox aPictureBox);

        // Construct and return a new instance of the game-specific GameLogic.
        IGameLogic GetNewGameLogic(GameState aState);

        // Construct and return a new instance of the game-specific GameState.
        GameState GetNewGameState();

        GameState GetNewGameState(GameBoard aBoard);

        // Given a game-specific board object, construct and return a new
        // game-specific BoardState message.
        GameMessage.BoardState GetNewBoardStateGameMessage(GameBoard aBoard);

        GameMessage.BoardState GetNewBoardStateGameMessage(string aBoardMsgString);
    }
}