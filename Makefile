all:
	g++ --std=c++14 -o octo-sync \
		src/main.cpp \
		src/Socket.cpp src/Server.cpp src/Client.cpp \
		src/StringQueue.cpp \
		src/logging.cpp \
		src/run.cpp src/FileUtils.cpp \
		src/ArgParser.cpp src/OctoSyncArgs.cpp \
		src/SyncDir.cpp \
		-pthread
