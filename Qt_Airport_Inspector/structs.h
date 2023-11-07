#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>
#include <thread>

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
