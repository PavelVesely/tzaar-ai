/* $Id: TzaarPainter.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The painter class takes a PictureBox as a parameter to its
 * constructor and latches onto it; then it draws the display within that box
 * whenever a game state is passed to the Draw method.
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
using System.Drawing.Drawing2D;

using System.Windows.Forms;
using DaedalusGameProtocol;
using TzaarGame;

namespace TzaarGame
{
    public class TzaarPainter : IGamePainter
    {
        protected int xOffset, yOffset;

        protected PictureBox pictureBox1;

        // UNSCALED CONSTANTS.
        protected abstract class BackgroundImageSizeInfo
        {
            public const int ColZeroX = 135;
            public const int ColZeroY = 805;

            // There are 109 pixels between the columns of the un-scaled game
            // board.
            public const int DistanceBetweenColumns = 109;

            // There are 124 pixels between the rows of the un-scaled game
            // board.
            public const int DistanceBetweenRows = 124;

            // Columns step up 65 pixels in columns 0 through 4, and then step
            // down 65 pixels in columns 5 through 9.
            public const int ColumnStep = 65;

            // Screen coordinates for piece counts.
            public static Point WhiteTzaarPoint = new Point(58, 56);
            public static Point WhiteTzarraPoint = new Point(58, 152);
            public static Point WhiteTottPoint = new Point(58, 246);
            public static Point BlackTzaarPoint = new Point(1073, 886);
            public static Point BlackTzarraPoint = new Point(1073, 981);
            public static Point BlackTottPoint = new Point(1073, 1077);
        }

        protected abstract class PieceImageSizeInfo
        {
            // The piece images are a different scale (larger) than the board
            // image, this is the ratio to scale them down to make them fit the
            // original board image.
            public const float PieceScaleFactor = 1.4f;
        }

        public class _CurrentImageInfo
        {
            public float Scale;
            public int ColZeroX;
            public int ColZeroY;
            public int DistanceBetweenColumns;
            public int DistanceBetweenRows;
            public int ColumnStep;
            public int PieceWidth;
            public int PieceTextXOffset;
            public int PieceTextYOffset;
            public Point WhiteTzaarPoint;
            public Point WhiteTzarraPoint;
            public Point WhiteTottPoint;
            public Point BlackTzaarPoint;
            public Point BlackTzarraPoint;
            public Point BlackTottPoint;
        }

        public _CurrentImageInfo CurrentImageInfo = new _CurrentImageInfo();

        public TzaarPainter(PictureBox aPictureBox)
        {
            this.pictureBox1 = aPictureBox;
            this.pictureBox1.Resize += pictureBox1_Resize;
            this.pictureBox1.Image = Properties.Resources.GreenTzaarBoard;
            UpdateScalingInfo();
        }

        // Calculate the current scale and pieceWidth (from the window size) for
        // the board display.
        private void UpdateScalingInfo()
        {
            // Calculate the graphics scaling ratio, depending on the current
            // window size.
            if (this.pictureBox1.Width > this.pictureBox1.Height)
            {
                this.CurrentImageInfo.Scale = (float)this.pictureBox1.Height / TzaarGame.Properties.Resources.GreenTzaarBoard.Width;
                this.yOffset = 0;
                this.xOffset = (this.pictureBox1.Width - this.pictureBox1.Height) / 2;
            }
            else
            {
                this.CurrentImageInfo.Scale = (float)this.pictureBox1.Width / TzaarGame.Properties.Resources.GreenTzaarBoard.Width;
                this.xOffset = 0;
                this.yOffset = (this.pictureBox1.Height - this.pictureBox1.Width) / 2;
            }

            this.CurrentImageInfo.PieceWidth = Convert.ToInt32(this.CurrentImageInfo.Scale * TzaarGame.Properties.Resources.WhiteTzaar.Width / PieceImageSizeInfo.PieceScaleFactor);
            this.CurrentImageInfo.ColZeroX = xOffset + Convert.ToInt32(BackgroundImageSizeInfo.ColZeroX * this.CurrentImageInfo.Scale);
            this.CurrentImageInfo.ColZeroY = yOffset + Convert.ToInt32(BackgroundImageSizeInfo.ColZeroY * this.CurrentImageInfo.Scale);
            this.CurrentImageInfo.DistanceBetweenColumns = Convert.ToInt32(BackgroundImageSizeInfo.DistanceBetweenColumns * this.CurrentImageInfo.Scale);
            this.CurrentImageInfo.DistanceBetweenRows = Convert.ToInt32(BackgroundImageSizeInfo.DistanceBetweenRows * this.CurrentImageInfo.Scale);
            this.CurrentImageInfo.ColumnStep = Convert.ToInt32(BackgroundImageSizeInfo.ColumnStep * this.CurrentImageInfo.Scale);
            this.CurrentImageInfo.PieceTextXOffset = Convert.ToInt32(this.CurrentImageInfo.Scale * 25.0f);
            this.CurrentImageInfo.PieceTextYOffset = Convert.ToInt32(this.CurrentImageInfo.Scale * 33.0f);
            this.CurrentImageInfo.WhiteTzaarPoint = new Point(this.xOffset + Convert.ToInt32(BackgroundImageSizeInfo.WhiteTzaarPoint.X * this.CurrentImageInfo.Scale), this.yOffset + Convert.ToInt32(BackgroundImageSizeInfo.WhiteTzaarPoint.Y * this.CurrentImageInfo.Scale));
            this.CurrentImageInfo.WhiteTzarraPoint = new Point(this.xOffset + Convert.ToInt32(BackgroundImageSizeInfo.WhiteTzarraPoint.X * this.CurrentImageInfo.Scale), this.yOffset + Convert.ToInt32(BackgroundImageSizeInfo.WhiteTzarraPoint.Y * this.CurrentImageInfo.Scale));
            this.CurrentImageInfo.WhiteTottPoint = new Point(this.xOffset + Convert.ToInt32(BackgroundImageSizeInfo.WhiteTottPoint.X * this.CurrentImageInfo.Scale), this.yOffset + Convert.ToInt32(BackgroundImageSizeInfo.WhiteTottPoint.Y * this.CurrentImageInfo.Scale));
            this.CurrentImageInfo.BlackTzaarPoint = new Point(this.xOffset + Convert.ToInt32(BackgroundImageSizeInfo.BlackTzaarPoint.X * this.CurrentImageInfo.Scale), this.yOffset + Convert.ToInt32(BackgroundImageSizeInfo.BlackTzaarPoint.Y * this.CurrentImageInfo.Scale));
            this.CurrentImageInfo.BlackTzarraPoint = new Point(this.xOffset + Convert.ToInt32(BackgroundImageSizeInfo.BlackTzarraPoint.X * this.CurrentImageInfo.Scale), this.yOffset + Convert.ToInt32(BackgroundImageSizeInfo.BlackTzarraPoint.Y * this.CurrentImageInfo.Scale));
            this.CurrentImageInfo.BlackTottPoint = new Point(this.xOffset + Convert.ToInt32(BackgroundImageSizeInfo.BlackTottPoint.X * this.CurrentImageInfo.Scale), this.yOffset + Convert.ToInt32(BackgroundImageSizeInfo.BlackTottPoint.Y * this.CurrentImageInfo.Scale));
        }

        // Takes a screen coordinate and returns a board position.
        public Point PictureBoxCoordsToBoardPosition(PointF p)
        {
            Point np = new Point();

            float x = (float)(p.X - CurrentImageInfo.ColZeroX) / (float)CurrentImageInfo.DistanceBetweenColumns;

            if (x < -0.5f)
            {
                throw new Exception("Click not on a piece: to the left of board.\n");
            }
            else if (x > 8.5f)
            {
                throw new Exception("Click not on a piece: to the right of board.\n");
            }

            np.X = (int)Math.Round(x);

            float y = p.Y - CurrentImageInfo.ColZeroY;

            // Now remove the column step.
            if (np.X < 5)
                y -= (float)CurrentImageInfo.ColumnStep * np.X;
            else
                y -= (float)CurrentImageInfo.ColumnStep * (8 - np.X);

            // Account for the hole in the middle of the board.
            if (np.X == 4 && Math.Abs(y) > (float)CurrentImageInfo.DistanceBetweenRows * 3.5f)
            {
                if (Math.Abs(y) < (float)CurrentImageInfo.DistanceBetweenRows * 4.5f)
                    // The piece is in the middle column, and above the (empty)
                    // board center.
                    throw new Exception("Click not on a piece: in the middle of board.\n");
                else
                    y += CurrentImageInfo.DistanceBetweenRows;
            }

            // Now divide by the space between rows to find the row.
            y /= (float)CurrentImageInfo.DistanceBetweenRows;

            if (y > 0.5f)
            {
                throw new Exception("Click not on a piece: below the bottom of board.\n");
            }
            else
            {
                float tx = -4.5f;

                if (p.X < 5)
                    tx -= np.X;
                else
                    tx -= (8 - np.X);

                if (p.X == 4)
                    tx += 1;

                if (y < tx)
                    throw new Exception("Click not on a piece: above the top of board.\n");
            }

            np.Y = (int)Math.Round(Math.Abs(y));

            return np;
        }

        // Takes a board position and returns the coordinate of the center of
        // that position on the screen.
        public PointF BoardPositionToPictureBoxCoords(Point p)
        {
            PointF np = new PointF();

            np.X = CurrentImageInfo.ColZeroX + (float)p.X * CurrentImageInfo.DistanceBetweenColumns;

            // Start at the offset of the bottom of the first column.
            np.Y = CurrentImageInfo.ColZeroY;

            // Subtract the distance between the rows multiplied by the index of
            // the desired row.
            if (p.X == 4 && p.Y >= 4)
                // The piece is in the middle column, and above the (empty)
                // board center.
                np.Y -= (float)(p.Y + 1) * CurrentImageInfo.DistanceBetweenRows;
            else
                np.Y -= (float)p.Y * CurrentImageInfo.DistanceBetweenRows;

            // Add the column step.
            if (p.X < 5)
                np.Y += CurrentImageInfo.ColumnStep * p.X;
            else
                np.Y += CurrentImageInfo.ColumnStep * (8 - p.X);

            return np;
        }

        protected PointF ApplyPieceTextOffset(PointF p)
        {
            return new PointF(p.X + CurrentImageInfo.PieceTextXOffset, p.Y + CurrentImageInfo.PieceTextYOffset);
        }

        protected PointF ApplyRectangleCornerOffset(PointF p)
        {
            int pieceRadius = CurrentImageInfo.PieceWidth / 2;
            return new PointF(p.X - pieceRadius, p.Y - pieceRadius);
        }

        private void DrawPiece(Graphics g, Image pieceImage, PointF p, string pieceCount, TzaarColor c)
        {
            // Find the position for the piece.
            PointF piecePos = ApplyRectangleCornerOffset(p);

            g.DrawImage(pieceImage, new RectangleF(piecePos.X, piecePos.Y, CurrentImageInfo.PieceWidth, CurrentImageInfo.PieceWidth));

            // Draw the pieceCount on the piece.
            using (StringFormat stringFormat = new StringFormat())
            {
                stringFormat.Alignment = StringAlignment.Center;
                stringFormat.LineAlignment = StringAlignment.Center;

                PointF countPos = ApplyPieceTextOffset(p);

                GraphicsPath path = new GraphicsPath();

                path.AddString(pieceCount, new FontFamily("Arial"), (int)FontStyle.Bold, 18.0f, new PointF(countPos.X, countPos.Y), stringFormat);

                if (c == TzaarColor.BLACK)
                {
                    // Fill in the outline.
                    g.FillPath(Brushes.Orange, path);
                    // Draw the outline.
                    g.DrawPath(new Pen(Color.DarkRed, 1.5f), path);
                }
                else
                {
                    // Fill in the outline.
                    g.FillPath(Brushes.LightGreen, path);
                    // Draw the outline.
                    g.DrawPath(new Pen(Color.Black, 1.5f), path);
                }
            }
        }

        private void DrawStack(Graphics g, Stack<TzaarPiece> S, float col, float row)
        {
            Image pieceImage;

            TzaarPiece p = (TzaarPiece)S.Peek();

            if (p.GetType() == typeof(TzaarPiece.Tzaar))
            {
                if (p.Color == TzaarColor.WHITE)
                    pieceImage = Properties.Resources.WhiteTzaar;
                else
                    pieceImage = Properties.Resources.BlackTzaar;
            }
            else if (p.GetType() == typeof(TzaarPiece.Tzarra))
            {
                if (p.Color == TzaarColor.WHITE)
                    pieceImage = Properties.Resources.WhiteTzarra;
                else
                    pieceImage = Properties.Resources.BlackTzarra;
            }
            else
            {
                if (p.Color == TzaarColor.WHITE)
                    pieceImage = Properties.Resources.WhiteTott;
                else
                    pieceImage = Properties.Resources.BlackTott;
            }

            PointF pieceLocation = BoardPositionToPictureBoxCoords(new Point((int)col, (int)row));

            if (p.Color == TzaarColor.WHITE)
                DrawPiece(g, pieceImage, new PointF(pieceLocation.X, pieceLocation.Y), S.Count.ToString(), TzaarColor.WHITE);
            else
                DrawPiece(g, pieceImage, new PointF(pieceLocation.X, pieceLocation.Y), S.Count.ToString(), TzaarColor.BLACK);
        }

        // Draw the words "Game Over" in big red letters on the PictureBox.
        private void DrawGameOver(Graphics g)
        {
            using (Font f = new Font("Arial", 34))
            using (SolidBrush b = new SolidBrush(Color.Red))
            using (StringFormat stringFormat = new StringFormat())
            {
                PointF center = new PointF(this.pictureBox1.Width / 2, this.pictureBox1.Height / 2);

                stringFormat.Alignment = StringAlignment.Center;
                stringFormat.LineAlignment = StringAlignment.Center;

                g.DrawString("Game Over", f, b, center, stringFormat);
            }
        }

        public void Draw(Graphics g, GameState gState)
        {
            TzaarGameState state = (TzaarGameState)gState;

            // Draw the score counts.
            DrawPiece(g, Properties.Resources.WhiteTzaar, CurrentImageInfo.WhiteTzaarPoint, state.Board.WhiteTzaarCount.ToString(), TzaarColor.WHITE);
            DrawPiece(g, Properties.Resources.WhiteTzarra, CurrentImageInfo.WhiteTzarraPoint, state.Board.WhiteTzarraCount.ToString(), TzaarColor.WHITE);
            DrawPiece(g, Properties.Resources.WhiteTott, CurrentImageInfo.WhiteTottPoint, state.Board.WhiteTottCount.ToString(), TzaarColor.WHITE);
            DrawPiece(g, Properties.Resources.BlackTzaar, CurrentImageInfo.BlackTzaarPoint, state.Board.BlackTzaarCount.ToString(), TzaarColor.BLACK);
            DrawPiece(g, Properties.Resources.BlackTzarra, CurrentImageInfo.BlackTzarraPoint, state.Board.BlackTzarraCount.ToString(), TzaarColor.BLACK);
            DrawPiece(g, Properties.Resources.BlackTott, CurrentImageInfo.BlackTottPoint, state.Board.BlackTottCount.ToString(), TzaarColor.BLACK);

            // Now draw the board graphics updates.
            for (int i = 0; i < 9; i++)
            {
                for (int j = 0; j < 9; j++)
                {
                    Stack<TzaarPiece> S = state.Board.Query(i, j);
                    if (S == null)
                        break;
                    else if (S.Count == 0)
                        continue;
                    else
                        // There is at least 1 piece on this location; display it.
                        this.DrawStack(g, S, i, j);
                }
            }

            if (state.GameIsOver)
                DrawGameOver(g);
        }

        // Called when the window gets resized.
        private void pictureBox1_Resize(object sender, EventArgs e)
        {
            this.UpdateScalingInfo();

            pictureBox1.Invalidate();
        }
    }
}