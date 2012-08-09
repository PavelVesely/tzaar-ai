/* $Id: GameMessage.Version.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The Version message is sent from the server to the client when
 * the client first connects.  It informs the client of the version of the Game
 * Manager, the name of the game the manager is configured to play, the version
 * of the game implementation, and (in games that support configurations
 * options).
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

namespace DaedalusGameProtocol
{
    public abstract partial class GameMessage
    {
        public class Version : GameMessage
        {
            private string managerVersion;
            private string gameName;
            private string gameConfig;
            private string gameVersion;

            public string ManagerVersion
            {
                get
                {
                    return this.managerVersion;
                }
            }

            public string GameName
            {
                get
                {
                    return this.gameName;
                }
            }

            public string GameVersion
            {
                get
                {
                    return this.gameVersion;
                }
            }

            public string GameConfig
            {
                get
                {
                    return this.gameConfig;
                }
            }

            public Version(string msg)
            {
                if (!GameMessage.IsVersion(msg))
                    throw new Exception("This is not a Version message.");

                // Remove all whitespace characters from the message string.
                string msgString = RemoveAllWhiteSpace(msg);

                this.data = msgString;

                string versionString = GameMessage.GetMessageData(msgString);

                string[] S = versionString.Split(';');
                if (S.Length != 4)
                    throw new Exception("This Version message is corrupt.");

                this.managerVersion = S[0];
                this.gameName = S[1];
                this.gameVersion = S[2];
                this.gameConfig = S[3];
            }

            public Version(string managerVersion, string gameName, string gameVersion, string gameConfig)
            {
                this.data = string.Format("{0}{{{1};{2};{3};{4}}}", typeof(GameMessage.Version).Name.ToString(), managerVersion, gameName, gameVersion, gameConfig);

                this.managerVersion = managerVersion;
                this.gameName = gameName;
                this.gameVersion = gameVersion;
                this.gameConfig = gameConfig;
            }
        }
    }
}