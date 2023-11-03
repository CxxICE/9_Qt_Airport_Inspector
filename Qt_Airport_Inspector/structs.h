#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>

#define MAX_THREADS_COUNT 4
#define MIN_THREADS_COUNT 1
#define EARLY_EXIT_MS 100

struct DBConnectionData
{
	QString host;
	int port;
	QString dbName;
	QString userName;
	QString password;
};

#endif // STRUCTS_H
