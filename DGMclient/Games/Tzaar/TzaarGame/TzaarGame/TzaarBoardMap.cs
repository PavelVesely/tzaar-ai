/* $Id: TzaarBoardMap.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The board map encapsulates the complexity of determining which
 * board positions connect to which other board positions.  It provides the
 * IsValidPath() method which returns true if a valid path exists between the
 * specified source and target positions, and false otherwise.
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

using System.Reflection;
using TzaarGame;

namespace TzaarGame
{
    public class TzaarBoardMap
    {
        // 2-D array used for node initiation. The board will be represented in
        // board[col][row] format.
        private Node[][] paths;

        public TzaarBoardMap()
            : base()
        {
            this.paths = new Node[9][]; // 9 columns.

            this.paths[0] = new Node[5];
            this.paths[1] = new Node[6];
            this.paths[2] = new Node[7];
            this.paths[3] = new Node[8];
            this.paths[4] = new Node[8];
            this.paths[5] = new Node[8];
            this.paths[6] = new Node[7];
            this.paths[7] = new Node[6];
            this.paths[8] = new Node[5];

            // Create new Node objects in the board array, named in (col,row)
            // format.
            for (int i = 0; i < this.paths.Length; i++)
                for (int j = 0; j < this.paths[i].Length; j++)
                    this.paths[i][j] = new Node(i, j);

            SetupBoardPaths(this.paths);
        }

        private static void SetupBoardPaths(Node[][] board)
        {
            // Link the nodes together.
            for (int i = 0; i < board.Length; i++)
            {
                for (int j = 0; j < board[i].Length; j++)
                {
                    CreateNorthAndSouthNodeLinks(board, i, j);
                    if (i < 4)
                        CreateLeftBoardLinks(board, i, j);
                    else if (i == 4)
                        CreateCenterBoardLinks(board, i, j);
                    else
                        CreateRightBoardLinks(board, i, j);
                }
            }
        }

        private static void CreateNorthAndSouthNodeLinks(Node[][] board, int col, int row)
        {
            // Create the northern link for a node.
            if (row != board[col].Length - 1 && !(col == 4 && row == 3))
                board[col][row].n = board[col][row + 1];

            // Create the southern link for a node.
            if (row != 0 && !(col == 4 && row == 4))
                board[col][row].s = board[col][row - 1];
        }

        private static void CreateLeftBoardLinks(Node[][] board, int col, int row)
        {
            // Create the northwest link for a node on the left side of the
            // board.
            if (col != 0 && row != board[col].Length - 1)
                board[col][row].nw = board[col - 1][row];

            // Create the southwest link for a node on the left side of the
            // board.
            if (col != 0 && row != 0)
                board[col][row].sw = board[col - 1][row - 1];

            // Create the northeast link for a node on the left side of the
            // board.
            if (col == 3 && row > 3)
                board[col][row].ne = board[col + 1][row];
            else if (col < 3 || (col == 3 && row < 3))
                board[col][row].ne = board[col + 1][row + 1];

            // Create the southeast link for a node on the left side of the
            // board.
            if (col == 3 && row > 4)
                board[col][row].se = board[col + 1][row - 1];
            else if (col < 3 || (col == 3 && row < 4))
                board[col][row].se = board[col + 1][row];
        }

        private static void CreateCenterBoardLinks(Node[][] board, int col, int row)
        {
            // Create the northwest and northeast links for a node in the center
            // column.
            if (row < 7)
            {
                if (row < 4)
                {
                    board[col][row].nw = board[col - 1][row];
                    board[col][row].ne = board[col + 1][row];
                }
                else
                {
                    board[col][row].nw = board[col - 1][row + 1];
                    board[col][row].ne = board[col + 1][row + 1];
                }
            }

            // Create the southwest and southeast links for node in the center
            // column.
            if (row > 0)
            {
                if (row < 4)
                {
                    board[col][row].sw = board[col - 1][row - 1];
                    board[col][row].se = board[col + 1][row - 1];
                }
                else
                {
                    board[col][row].sw = board[col - 1][row];
                    board[col][row].se = board[col + 1][row];
                }
            }
        }

        private static void CreateRightBoardLinks(Node[][] board, int col, int row)
        {
            // Create the northeast link for a node on the right side of the
            // board.
            if (row != board[col].Length - 1 && col != 8)
                board[col][row].ne = board[col + 1][row];

            // Create the southeast link for a node on the right side of the
            // board.
            if (row != 0 && col != 8)
                board[col][row].se = board[col + 1][row - 1];

            // Create the northwest link for a node on the right side of the
            // board.
            if (col == 5 && row > 3)
                board[col][row].nw = board[col - 1][row];
            else if (col > 5 || (col == 5 && row < 3))
                board[col][row].nw = board[col - 1][row + 1];

            // Create the southwest link for a node on the right side of the
            // board.
            if (col == 5 && row > 4)
                board[col][row].sw = board[col - 1][row - 1];
            else if (col > 5 || (col == 5 && row < 4))
                board[col][row].sw = board[col - 1][row];
        }

        // Start at the specified sourceNode and travel in the specified
        // direction.  Return true if there is a clear path between the two
        // specified board positions; otherwise return false.
        private static bool ExplorePath(TzaarBoard board, Node fromNode, Node toNode, string direction)
        {
            Node currentNode = fromNode;
            while (true)
            {
                // Move to the next node in the specified direction.
                FieldInfo myPropInfo = currentNode.GetType().GetField(direction);
                Node nextNode = (Node)myPropInfo.GetValue(currentNode);
                currentNode = nextNode;
                if (currentNode == null)
                    // We ran off the board.
                    break;

                if (currentNode == toNode)
                    // We have successfully reached our destination!
                    return true;

                Stack<TzaarPiece> currentPieceStack = board.Query(currentNode.col, currentNode.row);
                if (currentPieceStack.Count != 0)
                    // There are pieces on this position, stop here.
                    break;
            }

            // We can't get there.
            return false;
        }

        // Return true if there is some valid path from the specified source
        // position to the specified target position; otherwise return false.
        public bool IsValidPath(TzaarBoard board, int fromCol, int fromRow, int toCol, int toRow)
        {
            if (!board.IsValidPosition(fromCol, fromRow) || !board.IsValidPosition(toCol, toRow))
                // Parameters are out of range.
                return false;

            Node fromNode = paths[fromCol][fromRow];
            Node toNode = paths[toCol][toRow];

            if (fromCol == toCol)
            {
                if (fromRow == toRow)
                    // We can't move to ourselves!
                    return false;

                // The target is in the same column as the source.  So, we are
                // either going north or south.
                if (fromRow < toRow)
                    // Go north!
                    return ExplorePath(board, fromNode, toNode, "n");
                else
                    // Go south!
                    return ExplorePath(board, fromNode, toNode, "s");
            }
            else if (toCol > fromCol)
            {
                // Go east!

                // Try northeast first.
                if (ExplorePath(board, fromNode, toNode, "ne"))
                    return true;
                else
                    // Try southeast.
                    return ExplorePath(board, fromNode, toNode, "se");
            }
            else
            {
                // Go west!

                // Try northwest first.
                if (ExplorePath(board, fromNode, toNode, "nw"))
                    return true;
                else
                    // Try southwest.
                    return ExplorePath(board, fromNode, toNode, "sw");
            }
        }

        // Inner class representing the board space data structure.
        private class Node
        {
            // Neighbor pointers, represents possible moves.
            public Node n, s, nw, ne, sw, se;

            // Where am I?
            public int col, row;

            // Construct a new Node object.
            public Node(int col, int row)
            {
                this.col = col;
                this.row = row;

                n = s = nw = ne = sw = se = null;
            }
        }
    }
}