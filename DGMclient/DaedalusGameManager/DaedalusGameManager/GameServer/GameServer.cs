/* $Id: GameServer.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The game server instantiates a game, and listens for client
 * connections.  When a client connects, it instantiates a new
 * NetworkServerClientConnection class to maintain communications.  Once two
 * clients have connected, it starts the game.  The server then mediates
 * communication between the game logic, the game state, and the clients.
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
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using DaedalusGameProtocol;

namespace DaedalusGameManager
{
    // The Game NetworkServer.
    public class GameServer
    {
        // The game logic.
        private IGameLogic logic;

        // The Daedalus network server.
        private NetworkServer server;

        private int connectionCount;

        private int client1Id, client2Id;

        // Flag that indicates the GameServer is currently turned stopped.
        private volatile bool shutDown;

        // Flag that indicates the game is in progress.
        private volatile bool gameInProgress;

        private GameServerEventHandlerType gameStateUpdateEventHandler;

        // A constructor that takes a premade board as its parameter.
        public GameServer(int port, GameState state, GameServerEventHandlerType aHandler)
        {
            this.shutDown = false;
            this.gameInProgress = false;
            this.client1Id = -1;
            this.client2Id = -1;
            this.connectionCount = 0;

            this.gameStateUpdateEventHandler = aHandler;
            this.logic = GameChoice.CurrentGame.GetNewGameLogic(state);

            this.server = new NetworkServer(port, new NetworkServerEventHandlerType(ServerEventHandler));
        }

        private GamePlayer GetPlayerFromClientId(int clientId)
        {
            if (clientId == this.client1Id)
                return GamePlayer.One;
            else
                return GamePlayer.Two;
        }

        private int GetOtherClientId(int clientId)
        {
            if (clientId == this.client1Id)
                return this.client2Id;
            else
                return this.client1Id;
        }

        private int GetClientIdFromPlayer(GamePlayer player)
        {
            if (player == GamePlayer.One)
                return this.client1Id;
            else
                return this.client2Id;
        }

        // NetworkServer callback handler.
        private void ServerEventHandler(NetworkServerEventInfo info)
        {
            if (this.shutDown)
                // We have been told to shut down, don't start any new
                // connections now.
                return;

            if (info.Code == NetworkServerEventInfoCode.ClientConnected)
            {
                if (this.client1Id == -1)
                    this.client1Id = info.ClientId;
                else
                    this.client2Id = info.ClientId;

                // Send a Version message to the client.
                string managerVersion = this.GetType().Assembly.GetName().Version.ToString();
                string gameName = GameChoice.CurrentGame.GetGameName();
                string gameVersion = this.logic.GetType().Assembly.GetName().Version.ToString();

                string gameConfig = "";
                if (GameChoice.GameSupportsConfiguration(GameChoice.CurrentGame))
                {
                    // Get the configuration string for this game.
                    gameConfig = ((IGameConfig)GameChoice.CurrentGame).GetConfigString();
                }

                GameMessage.Version versionMsg = new GameMessage.Version(managerVersion, gameName, gameVersion, gameConfig);
                this.server.SendMessage(info.ClientId, versionMsg);

                // Call the upper layer's event handler to give it the good
                // news!
                GameServerEventInfo tinfo = new GameServerEventInfo(GameServerEventInfoCode.ClientConnected, "Remote Client ( " + this.server.GetClientRemoteEndPoint(info.ClientId).ToString() + " ) has connected.");
                this.gameStateUpdateEventHandler(tinfo);

                if (++this.connectionCount == 2)
                    // This is connection 2!  That means the game can begin!
                    StartGame();
            }
            else if (info.Code == NetworkServerEventInfoCode.ClientDisconnected)
            {
                // A client has disconnected.

                this.connectionCount--;

                if (info.ClientId == this.client1Id)
                    this.client1Id = -1;
                else
                    this.client2Id = -1;

                if (this.gameInProgress)
                {
                    this.logic.GetGameState().SetGameOver(GetPlayerFromClientId(GetOtherClientId(info.ClientId)), GameOverCondition.OpponentDisconnected);
                    EndGame();
                }
            }
            else if (info.Code == NetworkServerEventInfoCode.LogMessage)
            {
                CallEvent_LogMessage(info.Message);
            }
            else if (info.Code == NetworkServerEventInfoCode.NewData)
            {
                // A new message has been received.

                // Split multiple messages up on carriage return and linefeed.
                string[] msgs = info.Message.Split(new string[] { "\r\n" }, StringSplitOptions.RemoveEmptyEntries);

                // Add each new message to the read queue.
                foreach (string m in msgs)
                    ProcessNewMessageFromClient(info.ClientId, m);
            }
            else if (info.Code == NetworkServerEventInfoCode.ServerStopped)
            {
                CallEvent_LogMessage("NetworkServer stopped.");
            }
            else
            {
                // Should never be here!
                new Exception("NetworkServer generated an event with code " + info.Code);
            }
        }

        // Call the GUI handler so it can redraw the graphics, score, etc.
        private void CallEvent_Update(GameState state)
        {
            GameServerEventInfo info = new GameServerEventInfo(GameServerEventInfoCode.StateUpdated, state.Copy());
            this.gameStateUpdateEventHandler(info);
        }

        private void CallEvent_LogMessage(string msg)
        {
            GameServerEventInfo info = new GameServerEventInfo(GameServerEventInfoCode.LogMessage, msg);
            this.gameStateUpdateEventHandler(info);
        }

        private void CallEvent_ServerStopped()
        {
            // Tell the upper layer that we are all finished with shutdown now.
            GameServerEventInfo info = new GameServerEventInfo(GameServerEventInfoCode.ServerStopped, "NetworkServer stopped.");
            this.gameStateUpdateEventHandler(info);
        }

        // Stop the current game and the server.
        public void Stop()
        {
            if (gameInProgress)
            {
                this.logic.GetGameState().SetGameOver(GamePlayer.None, GameOverCondition.None);
                EndGame();
            }
            else
            {
                StopServer();
            }
        }

        private void StopServer()
        {
            if (this.shutDown)
                // We have already shutdown.  No need to do it again.
                return;

            this.shutDown = true;

            this.server.Stop();

            CallEvent_ServerStopped();
        }

        private void StartGame()
        {
            CallEvent_LogMessage("A new game has begun.");

            // Tell the clients their player numbers.
            this.server.SendMessage(this.client1Id, new GameMessage.YourPlayerNumber(GamePlayer.One));
            this.server.SendMessage(this.client2Id, new GameMessage.YourPlayerNumber(GamePlayer.Two));

            // Send the board's initial state to each client.
            GameBoard board = this.logic.GetGameState().Board.Copy();
            GameMessage initialBoardStateMessage = GameChoice.CurrentGame.GetNewBoardStateGameMessage(board);
            this.server.SendMessage(this.client1Id, initialBoardStateMessage);
            this.server.SendMessage(this.client2Id, initialBoardStateMessage);

            // Set that the game is now officially in progress.
            this.gameInProgress = true;

            // Send a YourTurn message to player One.
            this.server.SendMessage(this.client1Id, new GameMessage.YourTurn());

            CallEvent_LogMessage("Player One's turn.");
        }

        // End the game.
        private void EndGame()
        {
            GameOverCondition condition = this.logic.GetGameState().GameOverCondition;
            int winnerId = GetClientIdFromPlayer(this.logic.GetGameState().WinningPlayerNumber);
            int loserId = GetOtherClientId(winnerId);
            GameOverCondition condOne, condTwo;
            switch (condition)
            {
                case GameOverCondition.Draw:
                    // It's a draw!
                    CallEvent_LogMessage("Game Over: It's a draw!");
                    condOne = GameOverCondition.Draw;
                    condTwo = GameOverCondition.Draw;
                    break;
                case GameOverCondition.YouWin:
                case GameOverCondition.YouLose:
                    CallEvent_LogMessage("Game Over: Player " + GetPlayerFromClientId(winnerId) + " wins!!!!.");
                    condOne = GameOverCondition.YouWin;
                    condTwo = GameOverCondition.YouLose;
                    break;
                case GameOverCondition.OpponentDisconnected:
                    CallEvent_LogMessage("Game Over: Player " + GetPlayerFromClientId(loserId) + " disconnected.");
                    condOne = GameOverCondition.OpponentDisconnected;
                    condTwo = GameOverCondition.YouLose;
                    break;
                case GameOverCondition.OpponentMadeIllegalMove:
                case GameOverCondition.IllegalMove:
                    CallEvent_LogMessage("Game Over: Player " + GetPlayerFromClientId(loserId) + " made an illegal move.");
                    condOne = GameOverCondition.OpponentMadeIllegalMove;
                    condTwo = GameOverCondition.IllegalMove;
                    break;
                case GameOverCondition.OpponentResigned:
                    CallEvent_LogMessage("Game Over: Player " + GetPlayerFromClientId(loserId) + " resigned.");
                    condOne = GameOverCondition.OpponentResigned;
                    condTwo = GameOverCondition.YouLose;
                    break;
                default:
                    CallEvent_LogMessage("Game Over: Reason unknown!");
                    condOne = GameOverCondition.None;
                    condTwo = GameOverCondition.None;
                    break;
            }

            this.server.SendMessage(winnerId, new GameMessage.GameOver(condOne));

            this.server.SendMessage(loserId, new GameMessage.GameOver(condTwo));

            this.gameInProgress = false;

            // Stop the server.
            StopServer();

            // Notify the higher-ups.
            CallEvent_Update(this.logic.GetGameState());
        }

        private void ProcessNewMoveMessage(int clientId, string s)
        {
            // Try executing the command we received.
            try
            {
                this.logic.MakeMove(GetPlayerFromClientId(clientId), s);
            }
            catch (Exception)
            {
                // The move failed!
                CallEvent_LogMessage("Move from Player " + GetPlayerFromClientId(clientId) + " failed: " + s);
                if (!this.logic.GetGameState().GameIsOver)
                {
                    CallEvent_LogMessage("WARNING: Forcing GameOver Flag - This should probably be set by MakeMove() before it throws an Exception."); //TODO- Tzaar!
                    this.logic.GetGameState().SetGameOver(GetPlayerFromClientId(GetOtherClientId(clientId)), GameOverCondition.OpponentMadeIllegalMove);
                }
                EndGame();
                return;
            }

            // Click up to the next move (i.e. increment the counter, and
            // add this move to the move history) in the game.
            this.logic.GetGameState().AddMove(s);

            CallEvent_LogMessage("Player " + GetPlayerFromClientId(clientId) + " made move: " + s);

            // Forward the move along to the other player.
            this.server.SendMessage(GetOtherClientId(clientId), s);

            if (this.logic.GetGameState().GameIsOver)
            {
                // The game is over!
                EndGame();
                return;
            }

            // Notify the current player that it is their move.
            int nextMe = GetClientIdFromPlayer(this.logic.GetGameState().GetCurrentPlayerNumber());
            CallEvent_LogMessage("Player " + this.logic.GetGameState().GetCurrentPlayerNumber().ToString() + "'s turn.");
            this.server.SendMessage(nextMe, new GameMessage.YourTurn());

            // Call the GUI handler so it can redraw the graphics, score, etc.
            CallEvent_Update(this.logic.GetGameState());
        }

        private void ProcessNewMessageFromClient(int clientId, string s)
        {
            if (GameMessage.IsChat(s))
            {
                CallEvent_LogMessage("Player " + GetPlayerFromClientId(clientId) + " says: " + s);

                // Forward the chat message to the other player.
                this.server.SendMessage(GetOtherClientId(clientId), s);
            }
            else if (GameMessage.IsMove(s))
            {
                if (this.gameInProgress && this.logic.GetGameState().GetCurrentPlayerNumber() == GetPlayerFromClientId(clientId))
                    ProcessNewMoveMessage(clientId, s);
                else
                    // Junk message, ignore it.
                    CallEvent_LogMessage("Received unexpected Move message from Player " + GetPlayerFromClientId(clientId) + ": " + s);
            }
            else if (GameMessage.IsControl(s))
            {
                if (GameChoice.GameLogicSupportsControlMessages(this.logic))
                {
                    // Call the special "Control" operation in the game logic.
                    // Pass it a callback delegate.  The callback sends data to
                    // the specified client.  Afterwards, we check to see if the
                    // game is over, and tell the upper layers to redraw the
                    // graphics, etc.  Just in case something changed.

                    CallEvent_LogMessage("Control message from Player " + GetPlayerFromClientId(clientId) + ": " + s);

                    ((IGameControl)this.logic).Control(GetPlayerFromClientId(clientId), s, new ControlCallbackDelegate(ControlCallbackHandler));

                    if (this.logic.GetGameState().GameIsOver)
                    {
                        // The game is over!
                        EndGame();
                        return;
                    }

                    // Call the GUI handler so it can redraw the graphics,
                    // score, etc.
                    CallEvent_Update(this.logic.GetGameState());
                }
                else
                {
                    CallEvent_LogMessage("Received Control message from Player " + GetPlayerFromClientId(clientId) + ", but this game doesn't support Control messages: " + s);
                }
            }
            else
            {
                // Junk message, ignore it.
                CallEvent_LogMessage("Received junk message from Player " + GetPlayerFromClientId(clientId) + ": " + s);
            }
        }

        // The game logic calls this, via delegate in order to send responses to
        // Control{} messages.
        private void ControlCallbackHandler(GamePlayer playerNumber, string msg)
        {
            this.server.SendMessage(GetClientIdFromPlayer(playerNumber), msg);
        }
    }
}