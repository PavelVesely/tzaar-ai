/* $Id: TzaarClientPainter.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Extends the normal painter class to provide for client side
 * needs.
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
    public class TzaarClientPainter : TzaarPainter
    {
        private int selectionCol, selectionRow;
        private bool passButtonEnabled;
        private bool aPositionIsSelected;

        private TzaarColor myColor;

        private Rectangle passButtonRec = new Rectangle(900, 50, Properties.Resources.PassButton.Width, Properties.Resources.PassButton.Height);

        private Rectangle whiteLogoRec = new Rectangle(100 - (Properties.Resources.White.Width / 2), 1000 - (Properties.Resources.White.Height / 2), Properties.Resources.White.Width, Properties.Resources.White.Height);

        private Rectangle blackLogoRec = new Rectangle(100 - (Properties.Resources.Black.Width / 2), 1000 - (Properties.Resources.Black.Height / 2), Properties.Resources.Black.Width, Properties.Resources.Black.Height);

        private Rectangle yourColorLogoRec = new Rectangle(20, 900, Properties.Resources.YourColor.Width, Properties.Resources.YourColor.Height);

        private bool enableColorLogo;

        public bool APositionIsSelected
        {
            get
            {
                return this.aPositionIsSelected;
            }
        }

        public bool PassButtonEnabled
        {
            get
            {
                return this.passButtonEnabled;
            }
            set
            {
                this.passButtonEnabled = value;
            }
        }

        public TzaarClientPainter(PictureBox p)
            : base(p)
        {
            this.aPositionIsSelected = false;
            this.passButtonEnabled = false;
            this.enableColorLogo = false;
        }

        public void SetLocalPlayerColor(TzaarColor aColor)
        {
            this.myColor = aColor;
            this.enableColorLogo = true;
        }

        public new void Draw(Graphics g, GameState aState)
        {
            base.Draw(g, (TzaarGameState)aState);

            if (this.aPositionIsSelected)
                // Highlight a piece on the board.
                HighlightPosition(g, this.selectionCol, this.selectionRow);

            if (this.passButtonEnabled)
                // Draw a pass button.
                DrawPassButton(g);

            if (this.enableColorLogo)
                DrawLogo(g);
        }

        private void DrawLogo(Graphics g)
        {
            g.DrawImage(Properties.Resources.YourColor, ApplyScaleToRec(this.yourColorLogoRec));
            if (this.myColor == TzaarColor.WHITE)
                g.DrawImage(Properties.Resources.White, ApplyScaleToRec(this.whiteLogoRec));
            else
                g.DrawImage(Properties.Resources.Black, ApplyScaleToRec(this.blackLogoRec));
        }

        private void DrawPassButton(Graphics g)
        {
            g.DrawImage(Properties.Resources.PassButton, ApplyScaleToRec(this.passButtonRec));
        }

        private Rectangle ApplyScaleToRec(Rectangle rec)
        {
            int x = Convert.ToInt32(this.xOffset + rec.X * this.CurrentImageInfo.Scale);
            int y = Convert.ToInt32(this.yOffset + rec.Y * this.CurrentImageInfo.Scale);
            int width = Convert.ToInt32(rec.Width * this.CurrentImageInfo.Scale);
            int height = Convert.ToInt32(rec.Height * this.CurrentImageInfo.Scale);
            return new Rectangle(x, y, width, height);
        }

        private void HighlightPosition(Graphics g, int col, int row)
        {
            int highlightThickness = 5;

            PointF piecePos = BoardPositionToPictureBoxCoords(new Point((int)col, (int)row));

            piecePos = ApplyRectangleCornerOffset(piecePos);

            piecePos.X -= highlightThickness / 2;
            piecePos.Y -= highlightThickness / 2;

            g.DrawEllipse(new Pen(Brushes.LimeGreen, highlightThickness), new RectangleF(piecePos, new Size(this.CurrentImageInfo.PieceWidth, this.CurrentImageInfo.PieceWidth)));
        }

        // Return true if the coordinates are within the bounds of the "pass"
        // box.
        public bool IsPassClick(Point clickLoc)
        {
            Rectangle rec = ApplyScaleToRec(this.passButtonRec);
            if ((clickLoc.X >= rec.X)
                && (clickLoc.Y >= rec.Y)
                && (clickLoc.Y <= rec.Y + rec.Height)
                && (clickLoc.X <= rec.X + rec.Width))
                return true;
            else
                return false;
        }

        public void SelectPosition(int col, int row)
        {
            this.selectionCol = col;
            this.selectionRow = row;
            this.aPositionIsSelected = true;
        }

        public void ClearSelection()
        {
            this.aPositionIsSelected = false;
        }
    }
}