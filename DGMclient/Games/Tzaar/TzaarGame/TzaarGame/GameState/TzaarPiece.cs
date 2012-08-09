/* $Id: TzaarPiece.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: This class represents a game piece.  It has the color of the
 * piece (WHITE or BLACK) as an attribute.  There are three concrete subclasses
 * that each implement GamePiece, one for each type of game piece.
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
    public abstract class TzaarPiece
    {
        private TzaarColor color;
        public TzaarColor Color
        {
            get
            {
                return color;
            }
        }

        public TzaarPiece(TzaarColor c)
        {
            this.color = c;
        }

        public class Tzaar : TzaarPiece
        {
            public Tzaar(TzaarColor c)
                : base(c)
            {
            }
        }

        public class Tzarra : TzaarPiece
        {
            public Tzarra(TzaarColor c)
                : base(c)
            {
            }
        }

        public class Tott : TzaarPiece
        {
            public Tott(TzaarColor c)
                : base(c)
            {
            }
        }
    }
}