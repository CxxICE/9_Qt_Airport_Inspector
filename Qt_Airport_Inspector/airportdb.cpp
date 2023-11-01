#include "airportdb.h"

AirportDB::AirportDB(QObject *parent): QObject{parent}
{
	dataBase = new QSqlDatabase();
	airportsModel = new QSqlQueryModel(this);
	flightsModel = new QSqlQueryModel(this);
	yearsModel = new QSqlQueryModel(this);
	monthsModel = new QSqlQueryModel(this);
	queryArrivalsCount = nullptr;
	queryDeparturesCount = nullptr;
}

AirportDB::~AirportDB()
{
	delete dataBase;	
}

//НАСТРОЙКА И ПОДКЛЮЧЕНИЕ/ОТКЛЮЧЕНИЕ БД
/////////////////////////////////////////////////////////////////////////////////////////////////
void AirportDB::addDB(QString driver, QString nameDB)
{
	*dataBase = QSqlDatabase::addDatabase(driver, nameDB);
}

void AirportDB::connectDB(DBConnectionData data)
{
	if(dataBase->isOpen())
	{
		emit sig_SendError("БД уже открыта. Для подключения к другой БД отключитесь от текущей БД.");
	}
	else
	{
		dataBase->setHostName(data.host);
		dataBase->setDatabaseName(data.dbName);
		dataBase->setUserName(data.userName);
		dataBase->setPassword(data.password);
		dataBase->setPort(data.port);

		if(dataBase->open())
		{
			queryArrivalsCount = new QSqlQuery(*dataBase);
			queryDeparturesCount = new QSqlQuery(*dataBase);
			//подготовка запросов по статистике
			queryArrivalsCount->prepare("SELECT count(flight_no), date_trunc(:range, scheduled_arrival)::date as \"column_trunc\" "
												"FROM bookings.flights f "
												"WHERE (scheduled_arrival::date >= date(:from) and "
												"scheduled_arrival::date <= date(:to)) and "
												"(arrival_airport = :airport) "
												"GROUP BY \"column_trunc\" "
												"ORDER BY \"column_trunc\";");

			queryDeparturesCount->prepare("SELECT count(flight_no), date_trunc(:range, scheduled_departure)::date as \"column_trunc\" "
												"FROM bookings.flights f "
												"WHERE (scheduled_departure::date >= date(:from) and "
												"scheduled_departure::date <= date(:to)) and "
												"(departure_airport = :airport) "
												"GROUP BY \"column_trunc\" "
												"ORDER BY \"column_trunc\";");
			emit sig_SendStatusConnection(true);
		}
		else
		{
			emit sig_SendError(dataBase->lastError().text());
		}
	}
	return;
}

void AirportDB::disconnectDB(QString nameDb)
{
	dataBase->close();
	delete queryArrivalsCount;
	queryArrivalsCount = nullptr;
	delete queryDeparturesCount;
	queryDeparturesCount = nullptr;
	emit sig_SendStatusConnection(false);
}

bool AirportDB::isOpen()
{
	return dataBase->isOpen();
}


//МЕТОДЫ ПОЛУЧЕНИЯ СПИСКОВ АЭРОПОРТОВ, МЕСЯЦЕВ И ГОДОВ ДЛЯ МАППИРОВАНИЯ НА QCOMBOBOX
////////////////////////////////////////////////////////////////////////////////////
void AirportDB::getAirports()
{
	airportsModel->clear();
	airportsModel->setQuery("SELECT airport_name->>'ru' AS \"airportName\", airport_code "
							"FROM bookings.airports_data "
							"ORDER BY \"airportName\";",
							*dataBase);
	if(airportsModel->lastError().type() == 0)
	{
		emit sig_SendAirportsModel(airportsModel);
	}
	else
	{
		emit sig_SendError(airportsModel->lastError().text());
	}
	return;
}

void AirportDB::getMonths(int year)
{	
	monthsModel->clear();
	QString queryText = "SELECT EXTRACT(MONTH FROM scheduled_departure::DATE) as \"Month\" "
						"FROM bookings.flights f "
						"WHERE EXTRACT(YEAR FROM scheduled_departure::DATE) = %1 "
						"GROUP BY \"Month\" "
						"ORDER BY \"Month\";";
	monthsModel->setQuery(queryText.arg(QString::number(year)), *dataBase);

	if(monthsModel->lastError().type() == 0)
	{
		emit sig_SendMonthsModel(monthsModel);
	}
	else
	{
		emit sig_SendError(airportsModel->lastError().text());
	}
	return;
}

void AirportDB::getYears()
{
	yearsModel->setQuery("SELECT EXTRACT(YEAR FROM scheduled_departure::DATE) as \"Year\" "
						 "FROM bookings.flights f "
						 "group by \"Year\" "
						 "ORDER BY \"Year\";",
							*dataBase);
	if(yearsModel->lastError().type() == 0)
	{
		emit sig_SendYearsModel(yearsModel);
	}
	else
	{
		emit sig_SendError(airportsModel->lastError().text());
	}
	return;
}


//МЕТОДЫ ПОЛУЧЕНИЯ СПИСКОВ ПРИЛЕТАЮЩИХ И УЛЕТАЮЩИХ РЕЙСОВ ДЛЯ ЗАДАННОГО АЭРОПОРТА В ЗАДАННОМ ИНТЕРВАЛЕ ДАТ
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void AirportDB::getArrivals(QDate from, QDate to, QString airport)
{
	QString queryText = "SELECT flight_no, scheduled_arrival, ad.airport_name->>'ru' as \"Name\" from bookings.flights f "
					"JOIN bookings.airports_data ad on ad.airport_code = f.departure_airport "
					"WHERE (scheduled_arrival::date BETWEEN '%1' AND '%2') AND f.arrival_airport = '%3' ";
	flightsModel->setQuery(queryText.arg(from.toString("yyyy-MM-dd"),to.toString("yyyy-MM-dd"),airport), *dataBase);
	if(flightsModel->lastError().type() == 0)
	{
		emit sig_SendFlightsModel(flightsModel);
	}
	else
	{
		emit sig_SendError(flightsModel->lastError().text());
	}
	return;
}

void AirportDB::getDepartures(QDate from, QDate to, QString airport)
{
	QString queryText = "SELECT flight_no, scheduled_departure, ad.airport_name->>'ru' as \"Name\" from bookings.flights f "
					"JOIN bookings.airports_data ad on ad.airport_code = f.arrival_airport "
					"WHERE (scheduled_departure::date BETWEEN '%1' AND '%2') AND f.departure_airport = '%3' ";
	flightsModel->setQuery(queryText.arg(from.toString("yyyy-MM-dd"),to.toString("yyyy-MM-dd"),airport), *dataBase);
	if(flightsModel->lastError().type() == 0)
	{
		emit sig_SendFlightsModel(flightsModel);
	}
	else
	{
		emit sig_SendError(flightsModel->lastError().text());
	}
	return;
}

//МЕТОДЫ ПОЛУЧЕНИЯ МИНИМАЛЬНЫХ И МАКСИМАЛЬНЫХ ДАТ ПРИСУТСТВУЮЩИХ В БД
/////////////////////////////////////////////////////////////////////////////////

QDate AirportDB::getMinDateDeparture()
{	
	if(dataBase->isOpen())
	{
		QSqlQuery query(*dataBase);
		query.exec("SELECT MIN(scheduled_departure)::Date "
				   "FROM bookings.flights;");
		query.next();
		return query.value(0).toDate();
	}
	return QDate(1,0,0);
}

QDate AirportDB::getMaxDateDeparture()
{	
	if(dataBase->isOpen())
	{
		QSqlQuery query(*dataBase);
		query.exec("SELECT MAX(scheduled_departure)::Date "
				   "FROM bookings.flights;");
		query.next();
		return query.value(0).toDate();
	}
	return QDate(1,0,0);
}

QDate AirportDB::getMinDateArrival()
{	
	if(dataBase->isOpen())
	{
		QSqlQuery query(*dataBase);

		query.exec("SELECT MIN(scheduled_arrival)::Date "
				   "FROM bookings.flights;");
		query.next();
		return query.value(0).toDate();
	}
	return QDate(1,0,0);
}

QDate AirportDB::getMaxDateArrival()
{	
	if(dataBase->isOpen())
	{
		QSqlQuery query(*dataBase);

		query.exec("SELECT MAX(scheduled_arrival)::Date "
				   "FROM bookings.flights;");
		query.next();
		return query.value(0).toDate();
	}
	return QDate(1,0,0);
}


//МЕТОДЫ ПОЛУЧЕНИЯ КОЛИЧЕСТВА РЕЙСОВ ДЛЯ ЗАДАННОГО АЭРОПОРТА В ЗАДАННОМ ИНТЕРВАЛЕ ВРЕМЕНИ (ГОД ИЛИ МЕСЯЦ)
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void AirportDB::getStatisticMonth(QDate from, QDate to, QString airport)
{	
	getStatisticCommon(from, to, airport, "day");
}

void AirportDB::getStatisticYear(QDate from, QDate to, QString airport)
{
	getStatisticCommon(from, to, airport, "month");
}

//общий метод получения количества рейсов для заданного аэропорта в заданном интервале времени с заданной дискретностью (день, месяц) = range
void AirportDB::getStatisticCommon(QDate from, QDate to, QString airport, QString range)
{
	arrivalsCount.clear();
	departuresCount.clear();
	queryArrivalsCount->bindValue(":range", range);
	queryArrivalsCount->bindValue(":from", from.toString("yyyy-MM-dd"));
	queryArrivalsCount->bindValue(":to", to.toString("yyyy-MM-dd"));
	queryArrivalsCount->bindValue(":airport", airport);
	queryArrivalsCount->exec();
	while(queryArrivalsCount->next())
	{
		arrivalsCount.append(qMakePair(queryArrivalsCount->value(1).toDate(), queryArrivalsCount->value(0).toDouble()));
	}

	queryDeparturesCount->bindValue(":range", range);
	queryDeparturesCount->bindValue(":from", from.toString("yyyy-MM-dd"));
	queryDeparturesCount->bindValue(":to", to.toString("yyyy-MM-dd"));
	queryDeparturesCount->bindValue(":airport", airport);
	queryDeparturesCount->exec();

	while(queryDeparturesCount->next())
	{
		departuresCount.append(qMakePair(queryDeparturesCount->value(1).toDate(), queryDeparturesCount->value(0).toDouble()));
	}

	if(queryArrivalsCount->lastError().type() == 0 && queryDeparturesCount->lastError().type() == 0)
	{
		if(range == "day")
		{
			emit sig_SendStatisticMonth(&arrivalsCount, &departuresCount);
		}
		else if(range == "month")
		{
			emit sig_SendStatisticYear(&arrivalsCount, &departuresCount);
		}
	}
	else
	{
		emit sig_SendError(queryArrivalsCount->lastError().text() + '\n' + queryDeparturesCount->lastError().text());
	}

	return;
}





