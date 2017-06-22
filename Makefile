all:
	g++ --std=c++14 -pthread -o octo-sync src/main.cpp \
		src/Socket.cpp src/Server.cpp src/Client.cpp \
		src/logging.cpp \
		src/run.cpp \
		src/StringQueue.cpp