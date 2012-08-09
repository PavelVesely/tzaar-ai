/* $Id: GameChoice.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The GameChoice class is used by the DaedalusGameManagerForm to
 * determine what games are supported, to determine what the current game is,
 * and to switch the current game to a different game.
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
using DaedalusGameProtocol;
using TzaarGame;

namespace DaedalusGameManager
{
    public abstract class GameChoice
    {
        // The index within the Games[] array of the default game, i.e., the
        // game that is loaded when the game manager is first started.
        public const int IndexOfDefaultGame = 0;

        // The index within the Games[] array of the currently loaded game.
        private static volatile int currentGameIndex = IndexOfDefaultGame;
        public static IGame CurrentGame
        {
            get
            {
                return Games[currentGameIndex];
            }
        }

        // List of game interfaces.
        public static IGame[] Games = new IGame[]{
            new TzaarGameInterface(),
        };

        // This method is used to set the current game.
        public static void SetCurrentGame(int index)
        {
            currentGameIndex = index;
        }

        // Returns true if the specified IGame implementation also provides an
        // implementation of IGameConfig.
        public static bool GameSupportsConfiguration(IGame gameInterface)
        {
            return (gameInterface.GetType().GetInterface("IGameConfig") != null);
        }

        // Returns true if the specified IGameLogic implementation also
        // provides an implementation of IGameControl.
        public static bool GameLogicSupportsControlMessages(IGameLogic gameLogicInterface)
        {
            return (gameLogicInterface.GetType().GetInterface("IGameControl") != null);
        }
    }
}