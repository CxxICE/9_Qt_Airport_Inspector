#ifndef AIRPORTDB_H
#define AIRPORTDB_H

#include <QObject>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QDate>
#include <QDateTime>
#include <QThread>

#include "structs.h"

#define POSTGRE_DRIVER "QPSQL"
#define DB_NAME "AirportDB"

class AirportDB : public QObject
{
	Q_OBJECT
public:
	explicit AirportDB(QObject *parent = nullptr);
	~AirportDB();
	//методы настройки БД и подключения
	void addDB(QString driver, QString nameDB = "");
	void connectDB(DBConnectionData data);
	void disconnectDB(QString nameDb = "");	
	bool isOpen();

	//методы получения списков аэропортов, месяцев и годов для маппирования на QComboBox
	void getAirports();
	void getYears();
	void getMonths(int year);

	//методы получения списков прилетающих и улетающих рейсов для заданного аэропорта в заданном интервале дат
	void getArrivals(QDate from, QDate to, QString airport);
	void getDepartures(QDate from, QDate to, QString airport);

	//методы получения количества рейсов для заданного аэропорта в заданном интервале времени (год или месяц),
	//используются для получения статистики аэропорта по количеству рейсов
	//статистика за год - вывод количества рейсов по месяцам
	//статистика за месяц - вывод количества рейсов по дням
	//методы являются адаптерами над private методом getStatisticCommon
	void getStatisticYear(QDate from, QDate to, QString airport);
	void getStatisticMonth(QDate from, QDate to, QString airport);

	//методы получения минимальных и максимальных дат присутствующих в БД
	//для установки ограничений по датам вводимым пользователем в форме запроса
	//и для установки диапазонов запроса статистики по количеству рейсов из БД
	QDate getMinDateDeparture();
	QDate getMaxDateDeparture();
	QDate getMinDateArrival();
	QDate getMaxDateArrival();

signals:

   //сигналы с информацией о БД
   void sig_SendStatusConnection(bool status);   
   void sig_SendError(QString error);

   //сигналы с моделями для настройки QComboBox на основе данных в БД
   void sig_SendAirportsModel(QSqlQueryModel *airportsModel);
   void sig_SendYearsModel(QSqlQueryModel *yearsModel);
   void sig_SendMonthsModel(QSqlQueryModel *monthsModel, int year);

   //сигнал с моделью с данными о запрошенных перелетах
   void sig_SendFlightsModel(QSqlQueryModel *flightsModel);

   //сигналы с данными для построения графиков
   void sig_SendStatisticMonth(QVector<QPair<QDate, double>> *arrivalsCount, QVector<QPair<QDate, double>> *departuresCount);
   void sig_SendStatisticYear(QVector<QPair<QDate, double>> *arrivalsCount, QVector<QPair<QDate, double>> *departuresCount);

private:
	QSqlDatabase					*dataBase;

	QSqlQueryModel					*airportsModel;
	QSqlQueryModel					*flightsModel;
	QSqlQueryModel					*yearsModel;
	QSqlQueryModel					*monthsModel;

	QSqlQuery						*queryArrivalsCount;
	QSqlQuery						*queryDeparturesCount;

	QVector<QPair<QDate, double>>	arrivalsCount;
	QVector<QPair<QDate, double>>	departuresCount;

	//общий метод получения количества рейсов для заданного аэропорта в заданном интервале времени с заданной дискретностью (день, месяц)
	void getStatisticCommon(QDate from, QDate to, QString airport, QString range);
};

#endif // AIRPORTDB_H

