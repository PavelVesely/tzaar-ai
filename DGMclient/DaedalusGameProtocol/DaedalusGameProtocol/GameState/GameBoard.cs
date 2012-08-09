/* $Id: GameBoard.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: A concrete implementation of the abstract GameBoard holds a
 * snapshot of a board state.
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

namespace DaedalusGameProtocol
{
    public abstract class GameBoard
    {
        // Return a full copy of the game board.
        public abstract GameBoard Copy();
    }
}