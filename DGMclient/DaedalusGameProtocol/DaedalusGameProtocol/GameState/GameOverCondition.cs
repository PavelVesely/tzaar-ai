/* $Id: GameOverCondition.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The various end-of-game conditions.  Used by the protocol and
 * the game interface.
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
    public enum GameOverCondition
    {
        None = 0,
        YouWin = 1,
        YouLose = 2,
        OpponentDisconnected = 3,
        Draw = 4,
        IllegalMove = 5,
        OpponentResigned = 6,
        OpponentMadeIllegalMove = 7,
    }
}