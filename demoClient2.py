from pprint import pprint
from multiprocessing import Queue
import time
import socket
import sys
from threading import Thread, Lock
from blessed import *
import traceback
import select
'''
constants
'''
HOST = "127.0.0.1"
if(len(sys.argv) >= 2):
	HOST = sys.argv[1]
PORT = 50002
if(len(sys.argv) >= 3):
	PORT = sys.argv[2]

startTime = time.perf_counter()
onInterface = False
socketFinished = False

'''
global data structures
'''
trains = list()
trainSpeed = dict()
trainRunning = dict()
toSend = Queue()
serverLog = list()

'''
functions
'''
#Returns the number of seconds since the program started.
def addToLog(msg):
    serverLog.append(msg)
    print(msg)

def getUptime():
    return time.perf_counter() - startTime

def sendMsg(msg):
    #addToLog("Pushed to send queue: " + msg)
    toSend.put(msg)
    #with sendLock:
    #    server.send(msg.encode())

def sendSpeedUpdate(trainName, newSpeed):
    msg = "SPD " + trainName + " " + newSpeed + "\n"
    sendMsg(msg)

def playOrStopAll(newStatus):
    if newStatus == "PLAY" or newStatus == "STOP":
        if(len(trains) > 0):
            for train in trains:
                sendSpeedUpdate(train, newStatus)

def splitMsgBySpaces(msg):
    splice = []
    if ' ' in msg:
        splice = msg.split(' ')
    elif '_' in msg:
        splice = msg.split('_')
    return splice

def registerTrain(msg):
    splice = splitMsgBySpaces(msg)
    id = splice[1]
    speed = splice[2]
    trains.append(id)
    if ',' in speed:
        speed = speed.replace(',','.')
    trainSpeed[id] = float(speed)
    trainRunning[id] = True
    addToLog(id + " registered on controller")

def treatMsg(msg):
    decoded = msg
    if decoded.endswith('\n'):
        decoded = decoded[:-1]
    
    if 'REG' in decoded:
        addToLog("Received REG: " + decoded)
        registerTrain(decoded)
    else:
        addToLog("Received Undefined message: " + decoded)

def treatMsgs(messages):
    for msg in messages:
        treatMsg(msg)

def finish(sock):
    global socketFinished
    if socketFinished:
        addToLog("Already finished socket")
    else:
        addToLog("Finishing socket")
        sock.shutdown(socket.SHUT_RDWR)
        sock.close()
        socketFinished = True
        time.sleep(1)

def intListToBoolArray(intList):
    boolList = [None]*len(intList)
    #addToLog("boolList has space for " + str(len(boolList)))
    #addToLog("range is: ")
    #addToLog(range(0,len(intList)-1))
    for i in range(0,len(intList)):
        #addToLog(i)
        if(intList[i] > 0):
            boolList[i] = True
        else:
            boolList[i] = False
    return boolList

def connectionLoop(tcpSocket):
    inputs = list()
    inputs.append(tcpSocket)
    outputs = list()
    outputs.append(tcpSocket)
    sendQueue = toSend

    while inputs:
        global socketFinished
        if(socketFinished):
            addToLog("Before select: Socket finished, exiting select loop")
            break
        readable, writable, exceptional = select.select(inputs, outputs, inputs)
        for s in readable:
            try:
                #msg = s.recv(1024)
                msg = s.makefile().readlines()
            except socket.timeout as e:
                err = e.args[0]
                # this next if/else is a bit redundant, but illustrates how the
                # timeout exception is setup
                if err == 'timed out':
                    addToLog ('recv timed out, retry later')
                    sleep(0.01)
                    continue
                else:
                    addToLog('Bad timeout:')
                    addToLog(e)
                    finish(s)
            except socket.error as e:
                addToLog("Finishing because: Something else happened, handle error")
                addToLog(e)
                print(traceback.format_exception(None, e, e.__traceback__), file=sys.stderr, flush=True)
                finish(s)
            else:
                if len(msg) == 0:
                    addToLog('Finishing because: orderly shutdown on server end')
                    finish(s)
                else:
                    addToLog("readlines: " + str(msg))
                    treatMsgs(msg)

        if(socketFinished):
            addToLog("After readable: Socket finished, exiting select loop")
            break    
        for s in writable:
            try:
                if(not sendQueue.empty()):
                    next_msg = sendQueue.get_nowait()
                    #addToLog("Sending: " + next_msg)
                    s.send(next_msg.encode())
            except Exception as e:
                addToLog(e)
                addToLog("Finishing because of exception during msg sending")
                finish(s)

        if(socketFinished):
            addToLog("After writable: Socket finished, exiting select loop")
            break
        for s in exceptional:
            addToLog("Finishing because socket is in exceptional set")
            finish(s)

def getTrainX(selection):
    if(len(trains) >= selection+1):
        for train in trains:
            if(selection == 0):
                return train
            selection = selection - 1
    else:
        return ""

def printMenu(screenName, selection):
    if(screenName == "TRAINS"):
        return trainListScreen(selection)
    elif screenName == "MAIN":
        return mainMenu(selection)

def menuNItems(menuName):
    if menuName == "MAIN":
        return 5
    elif menuName == "TRAINS":
        if len(trains) > 0:
            return len(trains)
        else:
            return 1
    else:
        return 1

def exitOption(menuName, selection):
    if menuName == "MAIN" and selection == 4:
        return True
    elif menuName == "TRAINS":
        return True
    else:
        return False

def executeMenuOption(menuName, selection):
    selection = selection % menuNItems(menuName)
    if menuName == "MAIN":
        runMainMenuOption(selection)
    elif menuName == "TRAINS":
        runTrainsMenuOption(selection)

def runMainMenuOption(selection):
    if selection == 0:
        playOrStopAll("STOP")
    elif selection == 1:
        playOrStopAll("PLAY")
    elif selection == 2:
        cli("TRAINS")
    elif selection == 3:
        updateTrainSpeed("ALL")
    elif selection == 4:
        pass

def runTrainsMenuOption(selection):
    if(len(trains) > 0):
        updateTrainSpeed(getTrainX(selection))

def print_log(term):
    print('---- Log with ' + str(len(serverLog)) + ' messages ----')
    nLogs = 12
    for log in reversed(serverLog):
        if nLogs < 0:
            break
        print('{t.normal}{title}'.format(t=term, title=log))
        nLogs = nLogs-1

def updateTrainSpeed(trainName):
    if len(trains) == 0:
        return True
    newSpeed = 0.5
    term = Terminal()
    print(term.clear())
    text = "New speed for "
    if(trainName == "ALL"):
        text = text + "ALL: "
    elif(trainName == ""):
        return
    else:
        text = text + trainName + ": "
        newSpeed = trainSpeed[trainName]
    term.fullscreen()
    selecting = True
    with term.cbreak():
        while selecting:
            print(term.clear())
            print('{t.normal}{title}'.format(t=term, title=text))
            print('{t.normal}{title}'.format(t=term, title=str(newSpeed)))
            print('\n')
            print_log(term)
            key = term.inkey(timeout=0.5)
            if key.is_sequence:
                if key.name == 'KEY_DOWN':
                    newSpeed = newSpeed - 0.01
                    if(newSpeed < 0.0):
                        newSpeed = 0.0
                if key.name == 'KEY_UP':
                    newSpeed = newSpeed + 0.01
                    if(newSpeed > 1.0):
                        newSpeed = 1.0
                if key.name == 'KEY_ENTER':
                    selecting = False
    if trainName == "ALL" and len(trains) > 0:
        for train in trains:
            sendSpeedUpdate(train, str(newSpeed))
            trainSpeed[train] = newSpeed
    elif trainName in trains:
        sendSpeedUpdate(trainName, str(newSpeed))
        trainSpeed[trainName] = newSpeed

def display_train_list(selection, term):
    if(len(trains) > 0):
        for train in trains:
            text = "Train " + train + ": " 
            if trainRunning[train]:
                text = text + str(trainSpeed[train]) + "km/s."
            else:
                text = text + " stopped."
            if(selection == 0):
                print('{t.bold_red_reverse}{title}'.format(t=term, title=text))
            else:
                print('{t.normal}{title}'.format(t=term, title=text))
            selection = selection - 1
    else:
        print('{t.bold_red_reverse}{title}'.format(t=term, title="No trains registered"))

def trainListScreen(selection):
    if len(trains) == 0:
        return True
    selection = selection % len(trains)
    term = Terminal()
    print(term.clear())
    display_train_list(selection, term)
    print('\n')
    print_log(term)

mainMenuOptions = ["Stop all trains;", 
"Resume all trains;", 
"Change train speed;", 
"Change all trains speed;", 
"Exit;"]

def printMainMenuOptions(term, selection):
    for opt in mainMenuOptions:
        style = '{t.normal}{title}'
        if selection == 0:
            style = '{t.bold_red_reverse}{title}'
        print(style.format(t=term, title=opt))
        selection = selection - 1

def mainMenu(selection):
    selection = selection % 5
    term = Terminal()
    print(term.clear())
    printMainMenuOptions(term, selection)
    print('\n')
    print_log(term)

def cli(menuName):
    term = Terminal()
    exitCLI = False;
    while(not exitCLI):
        term.fullscreen()
        selection = 0
        selection_inprogress = True
        with term.cbreak():
            while selection_inprogress:
                printMenu(menuName, selection)
                key = term.inkey(timeout=0.5)
                if key.is_sequence:
                    if key.name == 'KEY_DOWN':
                        selection += 1
                    if key.name == 'KEY_UP':
                        selection -= 1
                    if key.name == 'KEY_ENTER':
                        selection_inprogress = False
                        addToLog("Choosen option " + str(selection))
                elif key == u'':
                    pass
            executeMenuOption(menuName, selection)
            if exitOption(menuName, selection):
                exitCLI = True
                break

'''
entry point
'''
addToLog("Host: " + HOST + "::" + str(PORT))
tcpSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
dest = (HOST, int(PORT))
try:
    #connect
    tcpSocket.connect(dest)
    tcpSocket.setblocking(0)
    thread = Thread(target = connectionLoop, args = (tcpSocket,))
    thread.start()
    onInterface = True
    cli("MAIN")
    onInterface = False

except socket.timeout:
    onInterface = False
    addToLog("Connection timeout")
except Exception as e:
    onInterface = False
    addToLog("Unknown error: ")
    addToLog(e)
    print(traceback.format_exception(None, e, e.__traceback__), file=sys.stderr, flush=True)

addToLog("Finishing because reached end of code")
finish(tcpSocket)