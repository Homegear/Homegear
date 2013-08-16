#ifndef CLICLIENT_H_
#define CLICLIENT_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <string>

namespace CLI {

class Client {
public:
	Client();
	virtual ~Client();

	void start();
private:
	int32_t _fileDescriptor;
	bool _stopPingThread = false;
	std::thread _pingThread;
	bool _closed = false;
	std::mutex _sendMutex;

	void ping();
};

} /* namespace CLI */
#endif /* CLICLIENT_H_ */
