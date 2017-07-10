all:
	g++ --std=c++11 -o bin/scp-bot \
		src/main.cpp \
		src/network/Socket.cpp src/network/Server.cpp src/network/Client.cpp \
		src/StringQueue.cpp \
		src/logging.cpp \
		src/run.cpp src/FileUtils.cpp \
		src/cli/ArgParser.cpp src/cli/SyncArgs.cpp \
		src/SyncBot/SyncBot.cpp src/SyncBot/SyncBotStates.cpp src/SyncBot/SyncBotMsgTreatment.cpp \
		src/SyncBot/SyncDir.cpp \
		-pthread
