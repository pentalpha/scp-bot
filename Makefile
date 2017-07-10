all:
	g++ --std=c++11 -o bin/scp-bot \
		src/main.cpp \
		src/Socket.cpp src/Server.cpp src/Client.cpp \
		src/StringQueue.cpp \
		src/logging.cpp \
		src/run.cpp src/FileUtils.cpp \
		src/ArgParser.cpp src/OctoSyncArgs.cpp \
		src/SyncBot.cpp src/SyncDir.cpp \
		-pthread
