import time
import socket
from threading import Thread
from multiprocessing import Queue
import sys

HOST = "127.0.0.1"
if(len(sys.argv) == 2):
	HOST = sys.argv[1]

PORT = 50002
if(len(sys.argv) == 3):
	PORT = sys.argv[2]
print("Host is in " + str(HOST) + "::" + str(PORT))

cmds = Queue()
cmds.put("delete /aaaa/bbbb/cccc/dddd.txt")
cmds.put("add /zzz/xxxxxx/eee/ggg.txt")
cmds.put("update /zzz/xxxxxx/eee/ggg.txt")

cmds.put("delete /aaaaa/bbbbb/cccc/ddddd.txt")
cmds.put("add /zzz/xxxxxx/eee/ggg.txt")
cmds.put("update /zzzzz/xxxxxx/eee/ggg.txt")

cmds.put("delete /aaaa/bbbbbb/cccc/dddd.txt")
cmds.put("add /zzzzzz/xxxxxx/eee/ggggg.txt")
cmds.put("update /zzz/xxxxxx/eeeeeee/ggg.txt")

tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
dest = (HOST, int(PORT))
tcp.connect(dest)

while not cmds.empty():
	bytes = cmds.get().encode()
	tcp.send(bytes)
	print("Sent: " + bytes.decode())
	time.sleep(0.1)
tcp.close()
