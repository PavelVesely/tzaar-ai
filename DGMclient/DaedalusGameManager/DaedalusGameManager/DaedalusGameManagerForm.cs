/* $Id: DaedalusGameManagerForm.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The main form instantiates a game server and a board painter to
 * display the game.  It handles user input, and mediates communication between
 * the server and the display.
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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using DaedalusGameProtocol;
using TzaarGame;

namespace DaedalusGameManager
{
    public partial class DaedalusGameManagerForm : Form
    {
        // The game server.
        private GameServer gameServer;

        // When game updates happen, the state snapshots are placed in this
        // queue by the server for our consumption.
        private Queue<GameState> gameStateQueue;

        // The current game state we received.
        private GameState currentGameState;

        private IGamePainter boardPainter;

        // For open/save dialogs.
        private string lastLoadFilePath;
        private string lastLoadFileName;
        private string lastSaveFilePath;
        private string lastSaveFileName;

        // StopServer and StartServer can be called asynchronously, so we employ
        // a simple lock and a flag to synchronize accesses.
        private object locker = new object();
        private volatile bool serverIsRunning;

        private PropertyGrid gamePropertyGrid;

        public bool ServerIsRunning
        {
            get
            {
                return serverIsRunning;
            }
        }

        // Start with a default starting board state.
        public DaedalusGameManagerForm()
            : this(null)
        {
        }

        // Load the specified board as the starting board state, or use the
        // default one if the parameter is null.
        public DaedalusGameManagerForm(GameBoard board)
        {
            InitializeComponent();

            this.serverIsRunning = false;
            this.gameStateQueue = new Queue<GameState>();

            this.lastLoadFilePath = Application.StartupPath;
            this.lastLoadFileName = "";

            this.lastSaveFilePath = Application.StartupPath;
            this.lastSaveFileName = "MySavedDaedalusGameBoard1.txt";

            // Populate the "Switch Game" menu item.
            for (int i = 0; i < GameChoice.Games.Length; i++)
                AddSwitchGameMenuItem(GameChoice.Games[i].GetGameName());

            // Load the default game.
            SwitchGames(GameChoice.IndexOfDefaultGame, board);

            this.Display("Welcome to the Daedalus Game Manager!\n");
            this.WindowState = FormWindowState.Minimized;
        }

        private void SetupInitialBoardState(GameBoard aBoard)
        {
            if (aBoard == null)
                this.currentGameState = GameChoice.CurrentGame.GetNewGameState();
            else
                this.currentGameState = GameChoice.CurrentGame.GetNewGameState(aBoard);

            pictureBox1.Invalidate();
        }

        private void SwitchGames(int gameIndex, GameBoard aBoard)
        {
            if (!Prompt_StopCurrentGame())
                return;

            StopServer();

            this.gameStateQueue.Clear();
            this.currentGameState = null;

            GameChoice.SetCurrentGame(gameIndex);

            this.boardPainter = GameChoice.CurrentGame.GetNewGamePainter(pictureBox1);

            if (GameChoice.GameSupportsConfiguration(GameChoice.CurrentGame))
            {
                this.gamePropertyGrid = ((IGameConfig)GameChoice.CurrentGame).GetPropertyGrid();
                ChangeConfigMenuItem(true, "");
            }
            else
            {
                // This game does not support configuration.
                this.gamePropertyGrid = null;
                ChangeConfigMenuItem(false, "This game does not support configuration.");
            }

            Display("Loaded Game: " + GameChoice.CurrentGame.GetGameName() + ".\n");

            SetupInitialBoardState(aBoard);

            // Set the appropriate checkbox in the menu.
            for (int i = 0; i < GameChoice.Games.Length; i++)
                if (i == gameIndex)
                {
                    ((ToolStripMenuItem)this.switchGameToolStripMenuItem.DropDownItems[i]).Checked = true;
                    ((ToolStripMenuItem)this.switchGameToolStripMenuItem.DropDownItems[i]).CheckState = CheckState.Checked;
                }
                else
                {
                    ((ToolStripMenuItem)this.switchGameToolStripMenuItem.DropDownItems[i]).Checked = false;
                    ((ToolStripMenuItem)this.switchGameToolStripMenuItem.DropDownItems[i]).CheckState = CheckState.Unchecked;
                }

            StartServer();
        }

        private void AddSwitchGameMenuItem(string gameName)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem();

            menuItem.Checked = false;
            menuItem.CheckState = CheckState.Unchecked;
            menuItem.Name = gameName + "ToolStripMenuItem";
            menuItem.Text = gameName;

            this.switchGameToolStripMenuItem.DropDownItems.Add(menuItem);
        }

        private void checkChangedEvent(object o, ToolStripItemClickedEventArgs e)
        {
            for (int i = 0; i < GameChoice.Games.Length; i++)
                if (GameChoice.Games[i].GetGameName().Equals(e.ClickedItem.Text))
                {
                    SwitchGames(i, null);
                    break;
                }
        }

        // Gets called by the server when we need to refresh the graphics.
        private void UpdateHandler(GameServerEventInfo info)
        {
            if (info.Code == GameServerEventInfoCode.StateUpdated)
            {
                this.gameStateQueue.Enqueue(info.State);

                if (info.State.GameIsOver)
                    StopServer();

                pictureBox1.Invalidate();
            }
            else if (info.Code == GameServerEventInfoCode.ServerStopped)
            {
                // NetworkServer stopped, clients are disconnected.
                StopServer();
            }

            if (info.Message != null)
                this.Display("> " + info.Message + "\n");
        }

        private void StopServer()
        {
            lock (this.locker)
                if (this.ServerIsRunning)
                {
                    this.serverIsRunning = false;
                    this.gameServer.Stop();
                }

            if (this.gamePropertyGrid != null)
                ChangeConfigMenuItem(true, "");

            ChangeStartServerMenuItemText("Start NetworkServer");
        }

        private delegate void ChangeMenuItemTextDelegate(string msg);
        private void ChangeStartServerMenuItemText(string msg)
        {
            if (this.menuStrip1.InvokeRequired)
            {
                ChangeMenuItemTextDelegate d = new ChangeMenuItemTextDelegate(ChangeStartServerMenuItemText);
                try
                {
                    this.Invoke(d, new object[] { msg });
                }
                catch
                {
                }
            }
            else
            {
                startServerToolStripMenuItem.Text = msg;
            }
        }

        private delegate void ChangeMenuItemDelegate(bool enable, string msg);
        private void ChangeConfigMenuItem(bool enable, string msg)
        {
            if (this.menuStrip1.InvokeRequired)
            {
                ChangeMenuItemDelegate d = new ChangeMenuItemDelegate(ChangeConfigMenuItem);
                try
                {
                    this.Invoke(d, new object[] { enable, msg });
                }
                catch
                {
                }
            }
            else
            {
                this.gameConfigurationToolStripMenuItem.ToolTipText = msg;
                this.gameConfigurationToolStripMenuItem.Enabled = enable;
            }
        }

        private void StartServer()
        {
            if (this.currentGameState.GameIsOver)
                // The current game is over, so generate a new board state.
                SetupInitialBoardState(null);

            int port = (int)DaedalusConfig.PortNumber;

            lock (this.locker)
                if (!this.ServerIsRunning)
                {
                    this.gameServer = new GameServer(port, this.currentGameState, UpdateHandler);
                    this.serverIsRunning = true;
                }

            this.Display("NetworkServer is listening for connections on port " + port + "....\n");

            if (this.gamePropertyGrid != null)
                ChangeConfigMenuItem(false, "You must stop the server in order to configure the game.");

            ChangeStartServerMenuItemText("Stop NetworkServer");
        }

        // Basically the idea here is that this function passes a pointer back
        // to itself, so that it can be called again from the proper context,
        // in due time.  This delegate is used to encapsulate the callback
        // pointer.
        private delegate void DisplayCallback(string s);

        private void Display(string s)
        {
            if (textBox_Status.InvokeRequired)
            {
                DisplayCallback d = new DisplayCallback(this.Display);
                try
                {
                    this.Invoke(d, new object[] { s });
                }
                catch
                {
                }
            }
            else
            {
                textBox_Status.AppendText(s);
                textBox_Status.SelectionStart = textBox_Status.TextLength;

                // This stuff is necessary to make sure the text window will
                // scroll down as we would expect it to.
                textBox_Status.ScrollToCaret();
            }
        }

        // Called when "Exit" is clicked from the menu.
        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.StopServer();
            this.Close();
        }

        // Called when "Start/Stop NetworkServer" is clicked.
        private void startServerToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (this.ServerIsRunning)
                this.StopServer();
            else
                this.StartServer();
        }

        // Return true if there is no game currently in progress; otherwise
        // popup a dialog asking if the current game should be ended: return
        // true to end game, false otherwise.
        private bool Prompt_StopCurrentGame()
        {
            if (this.ServerIsRunning)
                return (DialogResult.Yes == MessageBox.Show("There is a game in progress.  The game must be ended before I can continue.  Do you want me to end the current game?", "Stop Current Game?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning));

            return true;
        }

        private void saveBoardStateToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (currentGameState == null)
                return;

            saveFileDialog1.InitialDirectory = this.lastSaveFilePath;
            saveFileDialog1.FileName = this.lastSaveFileName;
            saveFileDialog1.Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*";
            saveFileDialog1.Title = "Save Board State";
            saveFileDialog1.DefaultExt = "txt";
            saveFileDialog1.OverwritePrompt = true;

            if (saveFileDialog1.ShowDialog() != DialogResult.Cancel)
            {
                string fileName = saveFileDialog1.FileName;

                try
                {
                    GameMessage.BoardState boardMsg = GameChoice.CurrentGame.GetNewBoardStateGameMessage(currentGameState.Board);
                    using (StreamWriter sw = new StreamWriter(fileName, false, Encoding.ASCII))
                        sw.Write(boardMsg);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(string.Format("Could not write to file '{0},' because I received an exception with message: {1}.", fileName, ex.Message), "File Read Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                // The file was successfully saved.

                this.lastSaveFilePath = Path.GetDirectoryName(fileName);
                this.lastSaveFileName = Path.GetFileName(fileName);

                this.Display(string.Format("Saved current board state to file '{0}.'\n", fileName));
            }
        }

        private void loadBoardToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!Prompt_StopCurrentGame())
                return;

            openFileDialog1.InitialDirectory = this.lastLoadFilePath;
            openFileDialog1.FileName = this.lastLoadFileName;
            openFileDialog1.Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*";
            openFileDialog1.Title = "Load Board State";
            openFileDialog1.DefaultExt = "txt";

            if (openFileDialog1.ShowDialog() != DialogResult.Cancel)
            {
                string fileName = openFileDialog1.FileName;
                GameMessage.BoardState boardMsg;

                try
                {
                    string boardMsgString;
                    using (StreamReader sr = new StreamReader(fileName, Encoding.ASCII, true))
                        boardMsgString = sr.ReadToEnd();

                    boardMsg = GameChoice.CurrentGame.GetNewBoardStateGameMessage(boardMsgString);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(string.Format("Could read a valid board state from file '{0},' because I received an exception with message: {1}.", fileName, ex.Message), "File Read Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                // The file was successfully loaded.

                this.lastLoadFilePath = Path.GetDirectoryName(fileName);
                this.lastLoadFileName = Path.GetFileName(fileName);

                this.StopServer();

                this.SetupInitialBoardState(boardMsg.Board);

                this.Display(string.Format("Loaded board state from file '{0}.'\n", fileName));
            }
        }

        private void resetToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!Prompt_StopCurrentGame())
                return;

            this.StopServer();

            this.SetupInitialBoardState(null);

            this.Display("Board state reset.\n");
        }

        // Called when someone clicks the 'X' to exit.
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            this.StopServer();
        }

        // This is where we actually do the screen updates.
        private void pictureBox1_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            g.CompositingQuality = CompositingQuality.HighQuality;
            g.SmoothingMode = SmoothingMode.HighQuality;

            // Process all the state snapshots on the queue.
            while (this.gameStateQueue.Count > 0)
            {
                GameState state = this.gameStateQueue.Dequeue();

                // Clear out the message queue on a game-switch.
                if (this.currentGameState != null && state.GetType() != this.currentGameState.GetType())
                    continue;

                // Process the state.
                this.currentGameState = state;
            }

            // Display the current game state.
            this.boardPainter.Draw(g, this.currentGameState);
        }

        private void gameConfigurationToolStripMenuItem_Click(object sender, EventArgs e)
        {
            GameConfigurationForm f = new GameConfigurationForm(this.gamePropertyGrid);
            f.ShowDialog();
        }

        private void optionsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            DaedalusGameManagerConfigurationForm f = new DaedalusGameManagerConfigurationForm();
            f.ShowDialog();
        }
    }
}