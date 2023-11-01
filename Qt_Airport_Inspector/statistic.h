#ifndef STATISTIC_H
#define STATISTIC_H

#include <limits>
#include <algorithm>

#include <QMainWindow>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QSplineSeries>
#include <QtCharts>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QChartView>
#include <QtConcurrent>
#include <QSemaphore>
#include <QAtomicInt>

#include "airportdb.h"

namespace Ui {
class Statistic;
}

class Statistic : public QMainWindow
{
	Q_OBJECT

public:
	Statistic(AirportDB *database, QWidget *parent = nullptr);
	~Statistic();

	//получение данных от MainWindow об аэропорте
	void setStatistic(QString _airportName, QString _airportCode);

	//методы для работы с Semaphore класса Statistic из MainWindow
	void acquireSemaphore(int i);
	void releaseSemaphore(int i);
	int availableSemaphores();

private slots:

	//слоты приема моделей о годах и месяцах доступных в БД
	void receiveYearsModel(QSqlQueryModel* yearsModel);
	void receiveMonthsModel(QSqlQueryModel* monthsModel);

	//слоты приема данных о статистике из БД
	void receiveStatisticMonth(QVector<QPair<QDate, double>> *arrivalsCount, QVector<QPair<QDate, double>> *departuresCount);
	void receiveStatisticYear(QVector<QPair<QDate, double>> *arrivalsCount, QVector<QPair<QDate, double>> *departuresCount);

	//слоты выбора месяцев и годов в QComboBox
	void on_cb_year_1_activated(int index);
	void on_cb_all_years_toggled(bool checked);
	void on_cb_year_2_activated(int index);
	void on_cb_month_2_activated(int index);

	//слоты выбора вкладки
	void receiveTabIndex(int index);

private:
	Ui::Statistic		*ui;

	//для работы с БД
	AirportDB			*airportDB;
	QString				airportCode;
	QSqlQueryModel		*yearsModel;
	QSemaphore			semaphore;

	//график статистики по месяцам года
	QStackedBarSeries	*yearGraph;
	QBarSet				*arrivalsSet;
	QBarSet				*departuresSet;
	QChart				*yearChart;
	QBarCategoryAxis	*xAxisYear;
	QValueAxis			*yAxisYear;
	QChartView			*yearChartView;

	//график статистики по дням месяца
	QLineSeries			*monthGraphArrivals;
	QLineSeries			*monthGraphDepartures;
	QLineSeries			*monthGraphSum;
	QChart				*monthChart;
	QValueAxis			*xAxisMonth;
	QValueAxis			*yAxisMonth;
	QChartView			*monthChartView;

	//счетчик созданных потоков в QtConcurrent --> при превышении макисмального числа потоков виджет блокирукется,
	//пока число потоков не снизится ниже минимального
	QAtomicInt counter;

	//вспомогательные методы преобразования названий месяцев в числа и обратно
	int MonthRusToInt(QString month);
	QString intToMonthRus(int month);

	//вспомогательный метод отсечения неполных месяцев
	void truncDays(QDate &from, QDate &to);

};

#endif // STATISTIC_H
