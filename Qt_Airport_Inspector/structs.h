#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>

struct DBConnectionData
{
	QString host;
	int port;
	QString dbName;
	QString userName;
	QString password;
};

#endif // STRUCTS_H
