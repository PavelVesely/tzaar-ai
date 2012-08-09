/* $Id: IGameLogic.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: An implementation of this interface is the game logic necessary
 * to play a particular game with the Game Manager.
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

namespace DaedalusGameProtocol
{
    public interface IGameLogic
    {
        GameState GetGameState();

        bool IsGameOver();

        void MakeMove(GamePlayer playerNumber, string msg);
    }
}