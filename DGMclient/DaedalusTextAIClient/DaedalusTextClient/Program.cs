/* $Id: Program.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: A simple text based client that connects to the Daedalus Game
 * Manager and allows the user to manually type and send Daedalus Game Manager
 * Protocol messages.
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
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using DaedalusGameProtocol;
using System.Diagnostics;
using System.Threading;

namespace DaedalusTextClient {
  class Program {
    static int pl; //what is me, 1 = white, -1 = black

    static int[] StandardBoard = {
	-1  ,  1  ,  1  ,  1  ,  1  , 100 , 100 , 100 , 100 ,
	-1  , -2  ,  2  ,  2  ,  2  , -1  , 100 , 100 , 100 ,
	-1  , -2  , -3  ,  3  ,  3  , -2  , -1  , 100 , 100 ,
	-1  , -2  , -3  , -1  ,  1  , -3  , -2  , -1  , 100 ,
	 1  ,  2  ,  3  ,  1  , 100 , -1  , -3  , -2  , -1  ,
	100 ,  1  ,  2  ,  3  , -1  ,  1  ,  3  ,  2  ,  1  ,
	100 , 100 ,  1  ,  2  , -3  , -3  ,  3  ,  2  ,  1  ,
	100 , 100 , 100 ,  1  , -2  , -2  , -2  ,  2  ,  1  ,
	100 , 100 , 100 , 100 , -1  , -1  , -1  , -1  ,  1
};
    static int[] board = new int[81];
    static int[] heights = new int[81];
    static int move = 0;

    static string DGMPath = @"DaedalusGameManager.exe";
    static string GUIClientPath = @"DaedalusGUIClient.exe"; 

    static Process pDGM, pGUI;

    static void Main(string[] args) {
      // set up output both to file and console
      Trace.Listeners.Clear();
      string stime = DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
      TextWriterTraceListener twtl = new TextWriterTraceListener(Path.Combine(OutputsDir, "DGMGame-" + stime + ".txt"));
      twtl.Name = "TextLoggerDGM";
      twtl.TraceOutputOptions = TraceOptions.ThreadId | TraceOptions.DateTime;

      ConsoleTraceListener ctl = new ConsoleTraceListener(false);
      ctl.TraceOutputOptions = TraceOptions.DateTime;

      Trace.Listeners.Add(twtl);
      Trace.Listeners.Add(ctl);
      Trace.AutoFlush = true;

      Trace.WriteLine("Welcome to the Daedalus Text Client.");
      // Start DGM and GUI client
      Trace.WriteLine("Starting the Daedalus Game Manager.");
      //pDGM = Process.Start(DGMPath); //for Windows
      pDGM = Process.Start("mono", DGMPath); //for Linux
      Thread.Sleep(5000); // to be sure that DGM has started

      bool white = ((new Random()).Next() % 2 == 0) ? true : false;
      if (!white) {
        Trace.WriteLine("Starting the Daedalus GUI Client.");
        //pGUI = Process.Start(GUIClientPath); //for Windows
        pGUI = Process.Start("mono", GUIClientPath); //for Linux
        Thread.Sleep(3000); // to be sure that GUI client is connected
      }

      // Acquire an IP address.
      Trace.WriteLine("Game Server IP address: 127.0.0.1");
      IPAddress host = IPAddress.Parse("127.0.0.1");//Trace.ReadLine());

      TcpClient client = new TcpClient(host.ToString(), 2525);
      client.NoDelay = true;

      if (!client.Connected)
        throw new Exception();

      Trace.WriteLine("Connected.\n");


      // Create our reader/writer streams.
      NetworkStream ns = client.GetStream();
      StreamWriter sw = new StreamWriter(ns);
      StreamReader sr = new StreamReader(ns);

      // Immediately flush data we put into the write buffer.
      sw.AutoFlush = true;

      bool shouldWait = false;
      // init board
      for (int i = 0; i < 81; i++) {
        board[i] = StandardBoard[i]; heights[i] = 0;
      }

      if (white) {
        Trace.WriteLine("Starting the Daedalus GUI Client.");
        //pGUI = Process.Start(GUIClientPath); //for Windows
        pGUI = Process.Start("mono", GUIClientPath); //for Linux
      }

      while (true) {
        String message;
        try {
          message = sr.ReadLine();
          if (message == "")
            continue;
        } catch (Exception) {
          Trace.WriteLine("Read Error.  Connection lost?");
          break;
        }

        if (GameMessage.IsVersion(message)) {
          // Version Info.
          Trace.WriteLine("Received Version Message: " + message);
        } else if (GameMessage.IsChat(message)) {
          // Chat message.
          Trace.WriteLine("Received Chat Message: " + message);
        } else if (GameMessage.IsBoardState(message)) {
          // Board State.
          Trace.WriteLine("Received Initial Board State: " + message);
          // supposing that there are no empty fields
          message += "BLACK WHITE Tzaar Tzarra Tott"; // because of searching for next occurence :)
          int pos = 0;
          int i = 0, j = 0;
          while (true) {
            if (board[i] != 100) {
              int color = 1;
              if (message.IndexOf("WHITE", pos) > message.IndexOf("BLACK", pos)) color = -1;
              int tzaar = message.IndexOf("Tzaar", pos);
              int tzarra = message.IndexOf("Tzarra", pos);
              int tott = message.IndexOf("Tott", pos);
              int min = Math.Min(Math.Min(tzaar, tzarra), tott);
              int val = 1;
              if (tzaar == min) val = 3;
              else if (tzarra == min) val = 2;
              board[i] = color * val;
              heights[i] = 1;
              pos = message.IndexOf('}', pos + 1) + 1;
              if (pos == -1) break;
            }
            i += 9;
            if (i >= 81) {
              j++;
              if (j == 9) break;
              i = j;
            }
          }

        } else if (GameMessage.IsMove(message)) {
          // Other player's move.
          Trace.WriteLine("Received: " + message);          
          DoMove(message);
          shouldWait = false;
        } else if (GameMessage.IsYourTurn(message)) {
          // Your Move.
          Trace.Write("YourMove> ");//column je row
          if (!shouldWait)
            PlayMove(sw);
          shouldWait = true;
        } else if (GameMessage.IsGameOver(message)) {
          // You Win.
          Trace.WriteLine("Game Over: " + new GameMessage.GameOver(message).Condition);
          Thread.Sleep(3000);
          break;
        } else if (GameMessage.IsYourPlayerNumber(message)) {
          // Your Player Number.
          pl = (int)new GameMessage.YourPlayerNumber(message).PlayerNumber;
          pl = -2 * pl + 3;
          Trace.WriteLine("You are player " + pl.ToString() + ": " + (pl == 1 ? "white" : "black"));
        } else {
          Trace.WriteLine("Unrecognized message!");
          break;
        }
      }
      sr.Close();
      sw.Close();
      client.Close();
      if (!pDGM.HasExited)
        pDGM.Kill();
      pDGM.Close();
      if (!pGUI.HasExited)
        pGUI.Kill();
      pGUI.Close();

      Trace.WriteLine("\nPress ESC key to exit:");
      while (true)
        if (Console.ReadKey().Key == ConsoleKey.Escape)
          break;
    }

    static void DoMove(string message) {
      string[] ints = message.Split(new char[] { ',', '{', '}' }, StringSplitOptions.RemoveEmptyEntries);
      if (ints.Length == 5) { // otherwise pass move
        int mfx, mfy, mtx, mty, mf, mt;
        mfx = int.Parse(ints[1]);
        mfy = int.Parse(ints[2]);
        mtx = int.Parse(ints[3]);
        mty = int.Parse(ints[4]);

        mf = mfy * 9 + mfx;
        if (mfx > 4) mf += (mfx - 4) * 9;
        else if (mfx == 4 && mfy > 3) mf += 9;

        mt = mty * 9 + mtx;
        if (mtx > 4) mt += (mtx - 4) * 9;
        else if (mtx == 4 && mty > 3) mt += 9;

        if (board[mf] * board[mt] < 0) {
          heights[mt] = heights[mf];
        }
        else {
          heights[mt] += heights[mf];
        }
        heights[mf] = 0;
        board[mt] = board[mf];
        board[mf] = 0;
      }
    }

    //Linux active, Windows inactive
    static readonly string LogsDir = @"./logs/";//@".\logs\";
    static readonly string OutputsDir = @"./outputs/";//@".\outputs\";
    static readonly string CurrPosFile = @"../../robot/currGame.txt";//@"..\..\robot\currGame.txt";
    //static readonly string ExecPosFile = @"..\..\currGameExec.txt";
    static String ExternalAIExec = @"../../robot/runTzaarLinux.sh"; //"cmd"; 
    static String ExternalAIArgs = @"";  //@"/c .\runTzaar.bat"; 

    static void PlayMove(StreamWriter sw) {
      //create file with pos
      move++;
      StreamWriter swpos = new StreamWriter(CurrPosFile);
      swpos.Write(pl.ToString());
      for (int i = 0; i < 81; i++) {
        if (i % 9 == 0) swpos.WriteLine();
        swpos.Write(board[i] + " ");
      }
      swpos.WriteLine();
      for (int i = 0; i < 81; i++) {
        if (i % 9 == 0) swpos.WriteLine();
        swpos.Write(heights[i] + " ");
      }
      swpos.Close();

      // copy file to logs
      string stime = DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
      string posLogFile = "DGMGame-" + stime + "-" + move.ToString("D2") + ".txt";
      File.Copy(CurrPosFile, Path.Combine(LogsDir, posLogFile));
      // start tzaar
      ProcessStartInfo pinfo = new ProcessStartInfo(Path.Combine(Directory.GetCurrentDirectory(), ExternalAIExec), Path.Combine(Directory.GetCurrentDirectory(), ExternalAIArgs));
      pinfo.UseShellExecute = false;
      pinfo.RedirectStandardOutput = true;
      Process p = Process.Start(pinfo);
      Thread.Sleep(1000);        
      StreamReader sr = p.StandardOutput;
      int ret = -1;
      string s;
      while ((s = sr.ReadLine()) != null) {
        Trace.WriteLine(s);
      }
      p.WaitForExit();
      TimeSpan processDuration = p.ExitTime - p.StartTime;
      ret = p.ExitCode;
      p.Dispose();  
      sr.Close();
      if (ret != 0) {
        Trace.WriteLine(string.Format("ERROR: External AI returned {0}", ret));
      }
      else {
        StreamReader output = File.OpenText(CurrPosFile);
        s = output.ReadToEnd();
        output.Close();
        stime = DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
        Trace.Write("output " + stime + ": ");
        Trace.Write(s);
        //string posLogFileExec = "DGMGame-" + stime + "-" + move.ToString("D2") + "-exec.txt";
        //File.Copy(CurrPosFile, Path.Combine(LogsDir, posLogFileExec));
        if ((s = File.ReadAllText(CurrPosFile)).StartsWith("END, SOMEONE HAS WON")) {
          Trace.WriteLine("win for " + (pl == 0 ? "white\n" : "black\n"));
        }
        string[] moves = s.Split(new char[] { ' ', '\n', '\r', '\t' }, StringSplitOptions.RemoveEmptyEntries);
		    int m1fx, m1fy, m1tx, m1ty, m2, m2fx = -1, m2fy = -1, m2tx = -1, m2ty = -1;
		    m1fx = (int)(moves[0][0]) - (int)('A');
		    m1fy = int.Parse(moves[0][1].ToString()) - 1;
		    m1tx = (int)(moves[1][0]) - (int)('A');
		    m1ty = int.Parse(moves[1][1].ToString()) - 1;
        string ms1 = "Move{" + string.Format("{0},{1},{2},{3}", m1fx, m1fy, m1tx, m1ty) + "}";
        sw.Write(ms1 + "\r\n");
        DoMove(ms1);
		    m2 = int.Parse(moves[2]);//0 == STACKING MOVE, 1 == CAPTURE, -1 == PASS, -2 == NOTHING (first move or win in one move)
		    if (m2 > -1) {
			    m2fx = (int)(moves[3][0]) - (int)('A');
			    m2fy = int.Parse(moves[3][1].ToString()) - 1;
			    m2tx = (int)(moves[4][0]) - (int)('A');
			    m2ty = int.Parse(moves[4][1].ToString()) - 1;
          string ms2 = "Move{" + string.Format("{0},{1},{2},{3}", m2fx, m2fy, m2tx, m2ty) + "}";
          sw.Write(ms2 + "\r\n");
          DoMove(ms2);
        }
        else if (m2 == -1) {
          sw.WriteLine("Move{}");
        }
      }


    }
  }
}