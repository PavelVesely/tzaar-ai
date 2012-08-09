/* $Id: TzaarColor.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The color (WHITE or BLACK) serves as both the player color and
 * the piece color.
 *
 * Copyright (c) 2010-2011, Team Daedalus (Mathew Bergt, Jason Buck, Ken Kelley, and
 * Justin Weaver).
 *
 * Distributed under the BSD-new license. For details see the BSD_LICENSE file
 * that should have been included with this distribution. If the source you
 * acquired this distribution from incorrectly removed this file, the license
 * may be viewed at http://www.opensource.org/licenses/bsd-license.php.
 */

using DaedalusGameProtocol;

namespace TzaarGame
{
    public enum TzaarColor
    {
        WHITE,
        BLACK,
    };

    public abstract class PlayerColor
    {
        public static GamePlayer GetPlayerFromColor(TzaarColor c)
        {
            return (c == TzaarColor.WHITE) ? GamePlayer.One : GamePlayer.Two;
        }

        public static TzaarColor GetColorFromPlayer(GamePlayer p)
        {
            return (p == GamePlayer.One) ? TzaarColor.WHITE : TzaarColor.BLACK;
        }
    }
}