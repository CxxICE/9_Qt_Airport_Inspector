#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>
#include <thread>

static const int MAX_THREADS_COUNT = std::thread::hardware_concurrency() - 1 <= 0 ?
                                                1 : std::thread::hardware_concurrency() - 1;
#define MIN_THREADS_COUNT 1
#define EARLY_EXIT_MS 50

struct DBConnectionData
{
	QString host;
	int port;
	QString dbName;
	QString userName;
	QString password;
};

#endif // STRUCTS_H
