/* $Id: IGameConfig.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: This is the interface through which games can set up their game
 * configurations options (variations in rules, board setup, etc).
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
    public interface IGameConfig
    {
        // Return a property grid with game-specific settings.
        PropertyGrid GetPropertyGrid();

        // Return a string representing the configurable rules of the game.
        // This string will be passed to the clients when they connect, so they
        // know the rules.
        string GetConfigString();
    }
}