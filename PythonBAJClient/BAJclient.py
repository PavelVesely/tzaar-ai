import time
import shutil
import subprocess
import os.path
import sys, traceback
import codecs
import smtplib
import base64
import requests # requires requests library

# settings
sleepSecs = 30
botUsername = '<bot username>'
botPassword = '<bot password>'
aiIndex = '42' # the best ai for the best robot
level = 'expert' # change according to the robot
emailTo = '<email to which messages about errors will be sent>'
emailFrom = '<email from which the messages will be sent>'
emailHeaders = """From: BAJ Tzaar client <%s>
To: Me <%s>
""" % (emailFrom, emailTo)
attachementMarker = "ASDFGHJKLQREWRWERQV"
attachementHeaders = """
MIME-Version: 1.0
Content-Type: multipart/mixed; boundary=%s
--%s
Content-Type: text/plain
Content-Transfer-Encoding:8bit
""" % (attachementMarker, attachementMarker)
attachementHeaders2 = """Content-Transfer-Encoding:base64
Content-Disposition: attachment; filename="""

# constants
true = 1
false = 0
userAgent = 'python-httplib'
bajPage = 'http://www.boiteajeux.net/'
loginPage = 'gestion.php'
nextGamePage = 'partiesuivante.php'
makeMovePage = 'jeux/tza/traitement.php'
joinGamePage = 'index.php?p=rejoindre'
fileForExternalAI = "<path to tzaar program>/BAJcurrGame.sav"
positionExecutedFile = "<path to tzaar program>/BAJposAfter.sav"
externalAIArgs = "-a " + aiIndex + " -e " + positionExecutedFile + "  -b " + fileForExternalAI
externalAIExec = "<path to tzaar program>/tzaar " + externalAIArgs 
logFile = "./log.txt"
logDir = "./<dir for the history positions>/"
outputDir = "./<dir for outputs>/"
outputFilePrefix = "output-"
StandardBoard = [	-1  ,  1  ,  1  ,  1  ,  1  , 100 , 100 , 100 , 100 ,
			-1  , -2  ,  2  ,  2  ,  2  , -1  , 100 , 100 , 100 ,
			-1  , -2  , -3  ,  3  ,  3  , -2  , -1  , 100 , 100 ,
			-1  , -2  , -3  , -1  ,  1  , -3  , -2  , -1  , 100 ,
			 1  ,  2  ,  3  ,  1  , 100 , -1  , -3  , -2  , -1  ,
			100 ,  1  ,  2  ,  3  , -1  ,  1  ,  3  ,  2  ,  1  ,
			100 , 100 ,  1  ,  2  , -3  , -3  ,  3  ,  2  ,  1  ,
			100 , 100 , 100 ,  1  , -2  , -2  , -2  ,  2  ,  1  ,
			100 , 100 , 100 , 100 , -1  , -1  , -1  , -1  ,  1 ]

# init cookie aware connection
session = requests.session()
requests.defaults.defaults['max_retries'] = 5

def getTimeString():
	ltime = time.localtime(time.time())
	return "%d-%.2d-%.2d_%.2d-%.2d-%.2d" % (ltime.tm_year, ltime.tm_mon, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec)

def getDateString():
	ltime = time.localtime(time.time())
	return "%d-%.2d-%.2d" % (ltime.tm_year, ltime.tm_mon, ltime.tm_mday)

def printOutput(str):
	output.write(str)
	output.write('\n')
	output.flush()

def writeOutput(str):
	output.write(str)
	output.flush()
	
def sendMail(subject, message, attachement):
	try:
		body = emailHeaders + "Subject: " + botUsername + ": " + subject
		body = body + "\n\n" + message
		if attachement != "":
			body = body + "\nAttachement: %s\n" % attachement
			fo = open(attachement, "r")
			body = body + fo.read()
		smtpObj = smtplib.SMTP('localhost')
		smtpObj.sendmail(emailFrom, emailTo, body)  
		printOutput("Successfully sent email")
	except Exception,e:
		printOutput("Error: unable to send email")
		exc_type, exc_value, exc_traceback = sys.exc_info()
		printOutput(e)
		traceback.print_tb(exc_traceback, limit=1, file=sys.stdout)
		printOutput("-- waiting due to exception")
		time.sleep(sleepSecs) 

def exception(err, subj, e, attachement):
	printOutput(err)
	exc_type, exc_value, exc_traceback = sys.exc_info()
	se = str(e)
	if se.find("HTTPConnectionPool") > -1:
		time.sleep(sleepSecs)
	else:
		sendMail(subj, err + "\n" + se, attachement)		
	printOutput(se)
	traceback.print_tb(exc_traceback, limit=1, file=sys.stdout)
	printOutput("-- waiting due to exception")
	time.sleep(sleepSecs)


def PlayGame(page):
	lines = page.splitlines()
	l = ""
	pl = 0; cpl = 1 # what player is bot? 1 == WHITE, -1 == BLACK 
	pIdCoup = ""
	idGame = 0
	for i in range(0,len(lines)):
		if lines[i].find("img/plateau.gif") > 0:
			if l != "":
				printOutput("ERROR: more lines with plateau")
				return
			l = lines[i]
		elif lines[i].find("onclick=\"detailjoueur") > 0:
			if lines[i].find(">" + botUsername + "<") > 0:
				pl = cpl
			else:
				cpl = -cpl
		elif lines[i].find("traitement.php?id=") > 0:
			#print "found traitement"
			start = lines[i].rfind('=') + 1
			idGame = int(lines[i][start : lines[i].rfind('"')])
		elif lines[i].find("name=\"pIdCoup\" value=\"") > 0:
			#print "id coup"
			start = lines[i].rfind('=') + 2
			pIdCoup = lines[i][start : lines[i].rfind('"')]
	if l == "":
		err = "ERROR: no line with plateau while playing %d" % idGame
		printOutput(err)
		sendMail("error loading pos", err, "")
		time.sleep(sleepSecs)
		return
	if pl == 0:
		err = "ERROR: cannot find bot username %s" % botUsername
		printOutput(err)
		sendMail("error loading pos", err, "")
		time.sleep(sleepSecs)
		return
	posFile = open(fileForExternalAI,"w")
	posFile.write("%d\n" % pl)
	elems = l.split("'img/")
	k = 0
	board = [0]*len(StandardBoard)
	heights = [0]*len(StandardBoard)
	waitingForHeight = false; middle = false
	stoneSum = 0
	for i in range(0,len(elems)):
		e = elems[i]
		while k < len(StandardBoard) and StandardBoard[k] == 100:
			posFile.write("100 ")
			board[k] = 100
			heights[k] = 0
			if (k == 40): middle = true # middle contains picture _.gif
			k = k + 1
			if (k % 9 == 0): posFile.write("\n")
			if (k >= len(StandardBoard)): break
		if (k >= len(StandardBoard)): break
		if (len(e) > 4 and e.startswith("_.gif")):
			if (not middle):
				board[k] = 0
				heights[k] = 0 
				posFile.write("0 ")
				k = k + 1
				if (k % 9 == 0): posFile.write("\n")
			else: middle = false
		if (len(e) > 5 and e[0].isdigit() and e[1].isdigit()):
			bstone = int(e[0])
			sbpl = int(e[1]) # 1 or 2
			spl = 1 - (sbpl - 1) * 2 # 1 or -1
			stone = spl * (4 - bstone)
			posFile.write("%d " % stone)
			if (waitingForHeight): 
				err = "ERROR: found stone before another stone got height k = %d while playing %d" % (k, idGame)
				printOutput(err)
				sendMail("error loading pos", err, "")
			board[k] = stone
			waitingForHeight = true
		if (len(e) > 4 and e.startswith("num") and e[3].isdigit()):
		  hs = e[3]
		  if (e[4].isdigit()): hs += e[4]
		  h = int(hs)
		  heights[k] = h
		  if (not waitingForHeight): 
		  	err = "ERROR: found height before stone k = %d while playing %d" % (k, idGame)
			printOutput(err)
			sendMail("error loading pos", err, "")
		  waitingForHeight = false
		  k = k + 1
		  if (k % 9 == 0): posFile.write("\n")
		  if (k >= len(StandardBoard)): break
	for i in range(0, len(heights)):
		if (i % 9 == 0): posFile.write("\n")
		posFile.write("%d " % heights[i])
	posFile.close()
	#logging
	logName = "BAJgame-" + str(idGame) + "_" + str(pl) + "_" + getTimeString() + ".txt"	
	printOutput("playing game, log: " + logName) # TODO print opponent?
	logPath = logDir + logName
	shutil.copy(fileForExternalAI, logPath)
	#sendMail("test", "att", logPath)
	try:
		p = subprocess.Popen(externalAIExec, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
		time.sleep(1)
		assertationFailed = false
		while 1:
			line = p.stdout.readline()
			if line == "": break
			if (line.find("ASSERTATION") > 0 or line.find("HashCollision") > 0) and assertationFailed == false:
				err = "output: %s\nwhen playing game %d, log name %s" % (line, idGame, logName)
				printOutput(err)
				sendMail("assertation failed", err, logPath)
			writeOutput(line)
		ret = p.wait()
		if (ret != 0):
			err = "ERROR: External AI returned %d when playing game %d, log name %s" % (ret, idGame, logName)
			printOutput(err)
			sendMail("error: bad exit code", err, logPath)
			time.sleep(sleepSecs)
			return
		output = open(fileForExternalAI,"r")
		s = output.read()
		output.close()
		writeOutput("output: " + s)
		moves = s.split()
		m2fx = -1; m2fy = -1; m2tx = -1; m2ty = -1
		m1fx = ord(moves[0][0]) - ord('A')
		m1fy = int(moves[0][1]) - 1
		if (m1fx > 4): m1fy += m1fx - 4
		elif (m1fx == 4 and m1fy > 3): m1fy += 1
		m1tx = ord(moves[1][0]) - ord('A')
		m1ty = int(moves[1][1]) - 1
		if (m1tx > 4): m1ty += m1tx - 4
		elif (m1tx == 4 and m1ty > 3): m1ty += 1
		m2 = int(moves[2])#0 == STACKING MOVE, 1 == CAPTURE, -1 == PASS, -2 == NOTHING (first move or win in one move)
		iVal = 3
		if (m2 > -1):
			m2fx = ord(moves[3][0]) - ord('A')
			m2fy = int(moves[3][1]) - 1
			if (m2fx > 4): m2fy += m2fx - 4
			elif (m2fx == 4 and m2fy > 3): m2fy += 1
			m2tx = ord(moves[4][0]) - ord('A')
			m2ty = int(moves[4][1]) - 1
			if (m2tx > 4): m2ty += m2tx - 4
			elif (m2tx == 4 and m2ty > 3): m2ty += 1
			iVal = 5
		#logging
		fileLog = open(logFile,"a")
		fileLog.write(logName + " " + str(moves[iVal]) + " " + str(moves[iVal + 1]) + "\n")  #TODO store moves?
		fileLog.close()
		
		#save position after executing best moves
		if (os.path.exists(positionExecutedFile)):
			shutil.copy(positionExecutedFile, logDir + "BAJgame-" + str(idGame) + "_" + str(pl) + "_" + getTimeString() + "-exec.txt")
		else:
			err = "ERROR: file with executed moves doesnt exist!"
			printOutput(err)
			sendMail("error", err, "")
			time.sleep(sleepSecs)
			return 
		sIdCoup = "name=\"pIdCoup\" value=\""
		
		data = {"pAction" : "choisirSource",
			"pL" : str(m1fx),
			"pC" : str(m1fy),
			"pIdCoup" : pIdCoup }
		
		response = session.post(bajPage + makeMovePage + "?id=" + str(idGame), data)
		page = response.text	
		start = page.find(sIdCoup) + len(sIdCoup)
		pIdCoup = page[start : page.find('"', start + 1)]
		
		data = {"pAction" : "destination",
			"pL" : str(m1tx),
			"pC" : str(m1ty),
			"pIdCoup" : pIdCoup }
		response = session.post(bajPage + makeMovePage + "?id=" + str(idGame), data)
				
		if (m2 != -2):		
			page = response.text
			start = page.find(sIdCoup) + len(sIdCoup)
			pIdCoup = page[start : page.find('"', start + 1)]
		
		if (m2 > -1):
			data = {"pAction" : "choisirSource",
				"pL" : str(m2fx),
				"pC" : str(m2fy),
				"pIdCoup" : pIdCoup }
			response = session.post(bajPage + makeMovePage + "?id=" + str(idGame), data)
			page = response.text		
			start = page.find(sIdCoup) + len(sIdCoup)
			pIdCoup = page[start : page.find('"', start + 1)]
			
			data = {"pAction" : "destination",
				"pL" : str(m2tx),
				"pC" : str(m2ty),
				"pIdCoup" : pIdCoup }
			response = session.post(bajPage + makeMovePage + "?id=" + str(idGame), data)
		
		elif (m2 == -1):
			data = {"pAction" : "passer",
				"pL" : str(m2fx),
				"pC" : str(m2fy),
				"pIdCoup" : pIdCoup }
			response = session.post(bajPage + makeMovePage + "?id=" + str(idGame), data)
		printOutput("done make move at %s" % getTimeString())
		
		#TODO
		#is it a position on which dfpns searched the game tree for more than 5 secs?
		#if (int.TryParse(moves[10], out val) and Math.Abs(val) == 1 and double.TryParse(moves[9], out time) and time > 5) { #TODO alpha-beta also can return +/-1
		#  File.Copy(logPath, Path.Combine(TestingPosDir, Path.GetFileName(logPath)))
		#  print "SAVING interesting dfpns position")
	except Exception,e:
		err = "ERROR:Exception during playing game %d, ret %d, log name %s" % (idGame, ret, logName)
		exception(err, "error during playing", e, logPath)

def Login():
	try:
		printOutput("Logging in ...")
		"""
		loginValues = {'p' : 'encours',
			'pAction' : 'login',
			'username' : botUsername,
			'password' : botUsername }
		"""
		response = session.post(bajPage + loginPage, "p=encours&pAction=login&username=" + botUsername + "&password=" + botPassword) # the data need to be in this format
		#print response.text
		if response.text.find('name="pAction" value="login">') > 0: # bot not logged on!!!
				err = "ERROR: Bot not logged on after login!"
	except Exception,e:
		err = "ERROR: Exception during loggin in"
		exception(err, "error during logging in", e, "")

# do not use standard output, write all to file
date = getDateString()
outputFile = outputDir + outputFilePrefix + date + ".txt"
print "setting output file to %s" % outputFile
output = open(outputFile, 'a+')

Login()

# polling loop
while 1:
	try:
		# output by a day
		ndate = getDateString()
		if ndate != date:
			sendMail("daily output", "output for day " + date, outputFile)
			date = ndate
			outputFile = outputDir + outputFilePrefix + date + ".txt"
			output = open(outputFile, 'a+')		
		foundGameInProgress = true
		while foundGameInProgress:
			response = session.get(bajPage + nextGamePage)
			page = response.text
			if page.find('name="pAction" value="login">') > 0: # bot not logged on
				printOutput("Bot not logged on!")
				sendMail("logging in again", "Bot not logged on!", "") # a test whether it can happen
				Login()
				response = session.get(bajPage + nextGamePage)
				page = response.text
				if page.find('name="pAction" value="login">') > 0:
					printOutput("ERROR: still not log in ...") 
					sendMail("error: not logged on after login", "ERROR: still not log in ...", "")
					time.sleep(sleepSecs)
			if page.find('<link rel="StyleSheet" type="text/css" href="tza.css">') > 0:
				PlayGame(page)
			elif page.find('title="You are invited to a game !">') > 0:
				response = session.get(bajPage + joinGamePage)
				page = response.text
				index = page.rfind("img/invites.gif") - 530
				sid = 'name="pAction" value="rejoindre"><input type="hidden" name="id" value="'
				idStart = page.find(sid, index) + len(sid)
				idEnd = page.find('"><', idStart) 
				id = page[idStart : idEnd]
				if id.startswith("tza-"):
					printOutput("joining game %s at %s" % (id, getTimeString()))
					data = {'pAction' : 'rejoindre', 'id' : id }
					response = session.post(bajPage + loginPage, data)					
				else:
					printOutput("BOT WAS INVITED TO BAD GAME :) " + id)
					foundGameInProgress = false # client should wait ...
					sendMail("error: bad game invitation", "bot was invited to game " + id, "")
					time.sleep(sleepSecs) 
			else:
				foundGameInProgress = false
		#print "nothing to play, waiting ... " + getTimeString()
		time.sleep(sleepSecs)
	except Exception,e:
		err = "ERROR: exception in polling loop"
		exception(err, "error in polling loop", e, "")

