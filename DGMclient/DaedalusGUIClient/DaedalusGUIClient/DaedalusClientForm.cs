/* $Id: DaedalusClientForm.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The GUI Client.  Maintains a copy of the game state, and updates
 * it whenever the local player makes a move.  It also updates the game state
 * when it receives a move that the server has forwarded from the other player.
 * Displays the game state, handles user interaction and board clicks, etc.
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

using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Windows.Forms;
using DaedalusGameProtocol;
using TzaarGame;

namespace DaedalusGUIClient
{
    public enum ClientStateCode
    {
        None,
        WaitingForConnection,
        WaitingForVersionMsg,
        WaitingForYourPlayerNumberMsg,
        WaitingForBoardStateMsg,
        WaitingForYourTurnMsg,
        GameIsInProgress,
        GameIsOver,
    }

    public partial class DaedalusClientForm : Form
    {
        private volatile ClientStateCode stateCode;

        private DaedalusClient client;

        // GameState queue w/lock
        private object locker = new object();
        private Queue<GameState> gameStateQueue;

        private GamePlayer playerNumber;

        private IGameClient gameClient;
        private IGameClientLogic gameClientLogic;

        private GameMessage.Version initVersionMsg;
        private GameMessage.BoardState initBoardStateMsg;

        public DaedalusClientForm()
        {
            InitializeComponent();

            ResetClientSideGameState();

            pictureBox1.SizeMode = PictureBoxSizeMode.Zoom;
            pictureBox1.Image = Properties.Resources.DaedalusClientLogo;

            this.gameStateQueue = new Queue<GameState>();

            this.client = new DaedalusClient(ConnectedCallbackHandler, NewMessageCallbackHandler);

            this.connectDisconnectButton_Click(this, EventArgs.Empty);
        }

        private void ResetClientSideGameState()
        {
            // We don't have a player number right now.
            this.playerNumber = (GamePlayer)(-1);

            this.stateCode = ClientStateCode.WaitingForConnection;
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
                textBox_Status.AppendText(s + "\n");
                textBox_Status.SelectionStart = textBox_Status.TextLength;

                // This stuff is necessary to make sure the text window will
                // scroll down as we would expect it to.
                textBox_Status.ScrollToCaret();
            }
        }

        private delegate void ChangeTextDelegate(string s);
        private void ChangeConnectButtonText(string s)
        {
            if (this.button_ConnectDisconnect.InvokeRequired)
            {
                ChangeTextDelegate d = new ChangeTextDelegate(ChangeConnectButtonText);
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
                this.button_ConnectDisconnect.Text = s;
            }
        }

        private void Connect()
        {
            if (this.stateCode != ClientStateCode.WaitingForConnection)
                return;

            IPAddress host;
            try
            {
                // Parse the contents of the server address box.
                host = IPAddress.Parse(this.textBox_IPAddress.Text);
            }
            catch (Exception ex)
            {
                // Parsing the server address failed.
                this.Display(ex.Message + "Please enter a valid IP address.");
                return;
            }

            int port = (int)this.numericUpDown_Port.Value;

            try
            {
                this.client.Connect(host, port);
            }
            catch (Exception ex)
            {
                this.Display(ex.Message + "Failed to connect.");
                return;
            }

            this.Display("Attempting to connect to " + host + " on port " + port + ".");

            ChangeConnectButtonText("Disconnect");

            ResetClientSideGameState();

            this.stateCode = ClientStateCode.WaitingForVersionMsg;

            this.pictureBox1.Invalidate();
        }

        private void ConnectedCallbackHandler()
        {
            if (!this.client.Connected)
            {
                // Disconnected!
                EndGame(GameOverCondition.Draw);
            }
            else
            {
                this.Display("Connected.  Waiting for game to begin...");
            }

            pictureBox1.Invalidate();
        }

        private bool InInitPhase()
        {
            return (this.stateCode == ClientStateCode.WaitingForConnection || this.stateCode == ClientStateCode.WaitingForYourPlayerNumberMsg || this.stateCode == ClientStateCode.WaitingForBoardStateMsg || this.stateCode == ClientStateCode.WaitingForVersionMsg);
        }

        private void NewMessageCallbackHandler(string msg)
        {
            // New data received.
            this.Display("> " + msg);

            if (GameMessage.IsVersion(msg))
            {
                // Version Info.
                GameMessage.Version versionMsg = new GameMessage.Version(msg);

                if (this.stateCode == ClientStateCode.WaitingForVersionMsg)
                {
                    this.stateCode = ClientStateCode.WaitingForYourPlayerNumberMsg;

                    this.Display("Received game manager Version message.");

                    this.Display("Version: " + versionMsg.ManagerVersion + " Game: '" + versionMsg.GameName + "' v" + versionMsg.GameVersion + " conf: '" + versionMsg.GameConfig + "'");

                    if (versionMsg.GameName.Equals("Tzaar"))
                        this.gameClient = new TzaarClientInterface();
                    else
                    {
                        this.Display("The game is not supported by this client!");
                        return;
                    }

                    this.initVersionMsg = versionMsg;
                }
                else if (this.stateCode == ClientStateCode.WaitingForYourPlayerNumberMsg || this.stateCode == ClientStateCode.WaitingForBoardStateMsg || this.stateCode == ClientStateCode.WaitingForVersionMsg)
                {
                    this.Display("(0) I did not receive the correct initialization sequence!");
                    ResetClientSideGameState();
                }
                else
                {
                    this.Display("Ignoring unexpected Version message: " + this.initVersionMsg);
                }
            }
            else if (GameMessage.IsChat(msg))
            {
                // Chat message.
                GameMessage.Chat chatMsg = new GameMessage.Chat(msg);

                this.Display("Your Opponent Says: " + msg);
            }
            else if (GameMessage.IsBoardState(msg))
            {
                if (this.stateCode == ClientStateCode.WaitingForBoardStateMsg)
                {
                    this.stateCode = ClientStateCode.GameIsInProgress;

                    this.initBoardStateMsg = this.gameClient.GetNewBoardStateGameMessage(msg);

                    this.gameStateQueue.Clear();

                    this.gameClientLogic = this.gameClient.GetNewGameClientLogic(this.pictureBox1, this.initVersionMsg, this.playerNumber, this.initBoardStateMsg.Board);

                    this.gameStateQueue.Enqueue(this.gameClientLogic.GetGameState());
                }
                else if (InInitPhase())
                {
                    this.Display("Received BoardState out of sequence!");
                    ResetClientSideGameState();
                }
                else
                {
                    this.Display("Ignoring unexpected BoardState message.");
                }
            }
            else if (GameMessage.IsYourPlayerNumber(msg))
            {
                if (this.stateCode == ClientStateCode.WaitingForYourPlayerNumberMsg)
                {
                    this.stateCode = ClientStateCode.WaitingForBoardStateMsg;

                    this.playerNumber = new GameMessage.YourPlayerNumber(msg).PlayerNumber;
                }
                else if (InInitPhase())
                {
                    this.Display("Received YourPlayerNumber out of sequence!");
                    ResetClientSideGameState();
                }
                else
                {
                    this.Display("Ignoring unexpected YourPlayerNumber message.");
                }
            }
            else if (GameMessage.IsYourTurn(msg))
            {
                GameMessage.YourTurn yourTurnMsg = new GameMessage.YourTurn(msg);
                this.Display("Received YourTurn message.");
            }
            else if (GameMessage.IsMove(msg))
            {
                if (this.stateCode == ClientStateCode.GameIsInProgress)
                {
                    GameMessage.Move newMove = this.gameClient.GetNewMoveGameMessage(msg);

                    RecordMove(this.playerNumber, newMove);
                }
                else
                {
                    this.Display("Ignoring unexpected Move message.");
                }
            }
            else if (GameMessage.IsGameOver(msg))
            {
                if (this.stateCode == ClientStateCode.GameIsInProgress)
                {
                    GameMessage.GameOver gameOverMsg = new GameMessage.GameOver(msg);

                    EndGame(gameOverMsg.Condition);
                }
                else
                {
                    this.Display("Ignoring unexpected GameOver message.");
                }
            }
            else
            {
                this.Display("Unrecognized command: " + msg);
            }

            this.pictureBox1.Invalidate();
        }

        private void RecordMove(GamePlayer playerNumber, GameMessage.Move move)
        {
            try
            {
                lock (this.locker)
                {
                    this.gameClientLogic.HandleMove(this.playerNumber, move);
                    this.gameStateQueue.Enqueue(this.gameClientLogic.GetGameState());
                }
            }
            catch (Exception)
            {
            }
        }

        private void EndGame(GameOverCondition condition)
        {
            if (this.stateCode != ClientStateCode.GameIsOver)
            {
                ClientStateCode lastStateCode = this.stateCode;
                this.stateCode = ClientStateCode.GameIsOver;

                if (lastStateCode > 0)
                {
                    this.Display("Disconnected.");

                    this.client.Disconnect();

                    if (lastStateCode == ClientStateCode.GameIsInProgress)
                        lock (this.locker)
                        {
                            this.gameClientLogic.EndGame(this.playerNumber, condition);
                            this.gameStateQueue.Enqueue(this.gameClientLogic.GetGameState());
                        }
                }

                ChangeConnectButtonText("Connect");
            }

            pictureBox1.Invalidate();
        }

        private void SendMove(string move)
        {
            this.Display("Sending Move: " + move);

            this.client.SendMessage(move);

            this.Display("Waiting for message from server...");

            pictureBox1.Invalidate();
        }

        // Handler for the event generated by pressing the enter key while the
        // 'IP Address' text box is in focus (cursor is in the 'IP Address' text
        // box). Starts the worker thread for a new connection.
        private void textBox_IPAddress_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Enter)
            {
                // The enter key was pressed.

                // Indicate the event is handled.
                e.Handled = true;
                Connect();

                // We don't handle disconnecting here because it wouldn't make
                // sense to disconnect when enter is pressed in the IP address
                // box.
            }
        }

        private void connectDisconnectButton_Click(object sender, EventArgs e)
        {
            if (this.client.Connected || this.client.Connecting)
            {
                EndGame(GameOverCondition.YouLose);
            }
            else
            {
                ResetClientSideGameState();
                Connect();
            }
        }

        // Handler for the event generated by closing the main form. Tells the
        // worker thread to shut down.
        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            EndGame(GameOverCondition.YouLose);
        }

        private void pictureBox1_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            g.CompositingQuality = CompositingQuality.HighQuality;
            g.SmoothingMode = SmoothingMode.HighQuality;

            GameState gameState = null;

            lock (this.locker)
            {
                while (this.gameStateQueue.Count > 1)
                    this.gameStateQueue.Dequeue();

                if (this.gameStateQueue.Count == 1)
                    gameState = this.gameStateQueue.Peek();
            }

            if (gameState != null)
                this.gameClientLogic.Paint(g, gameState);
        }

        private void pictureBox1_Click(object sender, MouseEventArgs e)
        {
            if (InInitPhase())
                return;

            GameMessage.Move move;
            try
            {
                move = this.gameClientLogic.HandleClick(this.playerNumber, new Point(e.X, e.Y));
            }
            catch
            {
                return;
            }

            if (move != null)
            {
                RecordMove(this.playerNumber, move);
                SendMove(move);
            }

            this.pictureBox1.Invalidate();
        }
    }
}