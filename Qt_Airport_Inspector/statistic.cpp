#include "statistic.h"
#include "ui_statistic.h"

#include <future>

Statistic::Statistic(AirportDB *database, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::Statistic)
{
	ui->setupUi(this);
	setWindowIcon(QIcon(":/img/airplane.ico"));
	ui->cb_year_1->setEnabled(false);
	ui->cb_year_2->setEnabled(false);
	ui->cb_month_2->setEnabled(false);
	ui->cb_all_years->setChecked(true);

	yearsModel = nullptr;
	airportDB = database;
	counterMonthsModel = 0;
	counterStatisticMonth = 0;
	counterStatisticYear = 0;
	semaphore.release(1);

	yearGraph = new QStackedBarSeries(this);
	arrivalsSet = new QBarSet("Прибывающие рейсы", this);
	departuresSet = new QBarSet("Вылетающие рейсы", this);
	yearChart = new QChart();
	yearChart->legend()->setVisible(true);
	yearChart->legend()->setAlignment(Qt::AlignBottom);
	xAxisYear = new QBarCategoryAxis(this);
	yAxisYear = new QValueAxis(this);
	yearChartView = new QChartView(yearChart);

	monthGraphArrivals = new QLineSeries(this);
	monthGraphDepartures = new QLineSeries(this);
	monthGraphSum = new QLineSeries(this);
	monthGraphArrivals->setName("Прибывающие рейсы");
	monthGraphDepartures->setName("Вылетающие рейсы");
	monthGraphSum->setName("Все рейсы");
	monthChart = new QChart();
	monthChart->legend()->setVisible(true);
	monthChart->legend()->setAlignment(Qt::AlignBottom);
	xAxisMonth = new QValueAxis(this);
	yAxisMonth = new QValueAxis(this);
	monthChartView = new QChartView(monthChart);

	ui->ch_year->addWidget(yearChartView);
	ui->ch_month->addWidget(monthChartView);	

	//принимаем результат запроса по существующим в БД годам в statisticWindow
	connect(airportDB, &AirportDB::sig_SendYearsModel, this, &Statistic::receiveYearsModel);
	//принимаем результат запроса по существующим в БД месяцам в statisticWindow
	connect(airportDB, &AirportDB::sig_SendMonthsModel, this, &Statistic::receiveMonthsModel);
	//принимаем результат запроса по количеству вылетов и прилетов в statisticWindow за месяц
	connect(airportDB, &AirportDB::sig_SendStatisticMonth, this, &Statistic::receiveStatisticMonth);
	//принимаем результат запроса по количеству вылетов и прилетов в statisticWindow за год
	connect(airportDB, &AirportDB::sig_SendStatisticYear, this, &Statistic::receiveStatisticYear);
	//принимаем сигнал смены вкладки для осуществления запроса к БД о статистике, для вывода на ней соответствующей диаграммы
	connect(this->ui->tabWidget, &QTabWidget::currentChanged, this, &Statistic::receiveTabIndex);
}

Statistic::~Statistic()
{
	delete ui;
	delete yearChart;
	delete monthChart;
}

//Прием основных параметров в окне Статистики (Название аэропорта, Код аэропорта)
//Запуск запроса в БД о статистике данного аэропорта
void Statistic::setStatistic(QString _airportName, QString _airportCode)
{
	yearsModel = nullptr;
	ui->cb_year_1->setEnabled(false);
	ui->cb_year_2->setEnabled(false);
	ui->cb_month_2->setEnabled(false);
	ui->cb_all_years->setChecked(true);
	ui->lb_airport->setText(_airportName + " " + _airportCode);	
	airportCode = _airportCode;
	auto f = QtConcurrent::run([&]()
	{
		semaphore.acquire(1);

		airportDB->getYears();

		semaphore.acquire(1);
		//после получения годов заправшиваем существующие месяца для последнего года существующего в БД
		int year = yearsModel->data(yearsModel->index(yearsModel->rowCount() - 1, 0)).toInt();
		airportDB->getMonths(year);

	});
}


//МЕТОДЫ ДЛЯ РАБОТЫ С SEMAPHORE КЛАССА STATISTIC ИЗ MAINWINDOW
void Statistic::acquireSemaphore(int i)
{
	semaphore.acquire(i);
}

void Statistic::releaseSemaphore(int i)
{
	semaphore.release(i);
}

int Statistic::availableSemaphores()
{
	return semaphore.available();
}

//СЛОТЫ ПРИЕМА МОДЕЛЕЙ
////////////////////////////////////////////////////////////////////

//слот приема модели с перечнем доступных в БД годов
void Statistic::receiveYearsModel(QSqlQueryModel *_yearsModel)
{
	ui->cb_year_1->setModel(_yearsModel);
	ui->cb_year_1->setModelColumn(0);
	ui->cb_year_2->setModel(_yearsModel);
	ui->cb_year_2->setModelColumn(0);	
	if(ui->cb_all_years->isChecked())
	{
		ui->cb_year_1->setEnabled(false);
	}
	else
	{
		ui->cb_year_1->setEnabled(true);
	}
	ui->cb_year_2->setEnabled(true);
	//выставляем текущим последний год из БД
	ui->cb_year_1->setCurrentIndex(_yearsModel->rowCount() - 1);
	ui->cb_year_2->setCurrentIndex(_yearsModel->rowCount() - 1);
	yearsModel = _yearsModel;

	semaphore.release(1);
}

//слот приема модели с перечнем месяцев доступных в БД по заданному году
void Statistic::receiveMonthsModel(QSqlQueryModel *monthsModel)
{
	qDebug()<<__FUNCTION__;
	QString curMonth;
	ui->cb_month_2->clear();
	int rows = monthsModel->rowCount();
	for (int y = 0; y < rows; ++y)
	{			
		curMonth = intToMonthRus(monthsModel->index(y,0).data().toInt());
		ui->cb_month_2->addItem(curMonth);
	}
	ui->cb_month_2->setCurrentIndex(0);
	ui->cb_month_2->setEnabled(true);

	if(ui->tabWidget->currentIndex() != 1)
	{
		ui->tabWidget->setCurrentIndex(1);
	}
	else
	{
		emit ui->cb_month_2->activated(ui->cb_month_2->currentIndex());
	}
	semaphore.release(1);

}

//СЛОТЫ ПРИЕМА СТАТИСТИКИ
///////////////////////////////////////////////////////////////////////////

//слот приема статистики за месяц
void Statistic::receiveStatisticMonth(QVector<QPair<QDate, double>> *arrivalsCount, QVector<QPair<QDate, double>> *departuresCount)
{
	if(counterStatisticMonth > EARLY_EXIT_COUNT)
	{
		semaphore.release(1);
		return;
	}
	if(monthChart->series().isEmpty() == false)
	{
		monthGraphArrivals->clear();
		monthGraphDepartures->clear();
		monthGraphSum->clear();
		monthChart->removeSeries(monthGraphArrivals);
		monthChart->removeSeries(monthGraphDepartures);
		monthChart->removeSeries(monthGraphSum);
		monthChart->removeAxis(xAxisMonth);
		monthChart->removeAxis(yAxisMonth);
	}

	//для настройки оси Y будем фиксировать минимальное и максимальное значение графика
	double minY = std::numeric_limits<double>::max();
	double maxY = 0;
	double yPoint;

	//график прилетов
	for (auto it = arrivalsCount->constBegin(); it != arrivalsCount->constEnd(); ++it)
	{
		if(counterStatisticMonth > EARLY_EXIT_COUNT)
		{
			semaphore.release(1);
			return;
		}
		yPoint = it->second;
		monthGraphArrivals->append(it->first.day(),yPoint);
		minY = minY < yPoint ? minY : yPoint;
	}

	//график вылетов
	for (auto it = departuresCount->constBegin(); it != departuresCount->constEnd(); ++it)
	{
		if(counterStatisticMonth > EARLY_EXIT_COUNT)
		{
			semaphore.release(1);
			return;
		}
		yPoint = it->second;
		monthGraphDepartures->append(it->first.day(),yPoint);
		minY = minY < yPoint ? minY : yPoint;
	}

	//суммарный график, треубет суммирования данный по соответствующим месяцам
	//определяем в каких данных (по прилетам или вылетам больше точек), они могу не совпадать, вероятен простой аэропорта
	auto biggerCount = departuresCount->count() > arrivalsCount->count() ? departuresCount : arrivalsCount;
	auto lessCount = departuresCount->count() > arrivalsCount->count() ? arrivalsCount : departuresCount;

	//суммируем данные по датам
	auto it_l = lessCount->constBegin();
	auto it_b = biggerCount->constBegin();
	while(it_l < lessCount->constEnd() && it_b < biggerCount->constEnd())
	{
		if(counterStatisticMonth > EARLY_EXIT_COUNT)
		{
			semaphore.release(1);
			return;
		}
		if(it_b->first.day() == it_l->first.day())
		{
			yPoint = it_l->second + it_b->second;
			monthGraphSum->append(it_b->first.day(), yPoint);
			minY = minY < yPoint ? minY : yPoint;
			maxY = maxY > yPoint ? maxY : yPoint;
			++it_l;
			++it_b;
		}
		else if(it_b->first.day() > it_l->first.day())
		{
			yPoint = it_l->second;
			monthGraphSum->append(it_l->first.day(), yPoint);
			minY = minY < yPoint ? minY : yPoint;
			maxY = maxY > yPoint ? maxY : yPoint;
			++it_l;
		}
		else
		{
			yPoint = it_b->second;
			monthGraphSum->append(it_b->first.day(), it_b->second);
			minY = minY < yPoint ? minY : yPoint;
			maxY = maxY > yPoint ? maxY : yPoint;
			++it_b;
		}
	}
	//если какой-либо массив не был пройден до конца, дозаписываем его в график
	if(it_l == lessCount->constEnd() && it_b != biggerCount->constEnd())
	{
		do
		{
			if(counterStatisticMonth > EARLY_EXIT_COUNT)
			{
				semaphore.release(1);
				return;
			}
			yPoint = it_b->second;
			monthGraphSum->append(it_b->first.day(), it_b->second);
			minY = minY < yPoint ? minY : yPoint;
			maxY = maxY > yPoint ? maxY : yPoint;
			++it_b;
		}while(it_b < biggerCount->constEnd());
	}
	else if(it_l != lessCount->constEnd() && it_b == biggerCount->constEnd())
	{
		do
		{
			if(counterStatisticMonth > EARLY_EXIT_COUNT)
			{
				semaphore.release(1);
				return;
			}
			yPoint = it_l->second;
			monthGraphSum->append(it_l->first.day(), it_l->second);
			minY = minY < yPoint ? minY : yPoint;
			maxY = maxY > yPoint ? maxY : yPoint;
			++it_l;
		}while(it_l < lessCount->constEnd());
	}

	//настраиваем график и выводим на экран
	monthChart->addSeries(monthGraphArrivals);
	monthChart->addSeries(monthGraphDepartures);
	monthChart->addSeries(monthGraphSum);

	yAxisMonth->setTickType(QValueAxis::TicksDynamic);
	int interval = 1 > (maxY - minY)/10 ? 1 : (maxY - minY)/10;
	yAxisMonth->setTickInterval(interval);
	yAxisMonth->setRange(static_cast<int>(0.9 * minY), 1.1 * maxY);
	yAxisMonth->setTickAnchor(0);
	yAxisMonth->setLabelFormat("%d");

	//вычисление дней в месяце для оси X, с проверкой на пустоту arrivalsCount и departuresCount
	int month = 31;
	if(arrivalsCount->count() > 0)
	{
		month = arrivalsCount->at(0).first.daysInMonth();
	}
	else if (departuresCount->count() > 0)
	{
		month = departuresCount->at(0).first.daysInMonth();
	}

	xAxisMonth->setRange(1, month);
	xAxisMonth->setTickCount(month);
	xAxisMonth->setLabelFormat("%d");

	monthChart->addAxis(xAxisMonth, Qt::AlignBottom);
	monthChart->addAxis(yAxisMonth, Qt::AlignLeft);
	monthGraphSum->attachAxis(xAxisMonth);
	monthGraphSum->attachAxis(yAxisMonth);
	monthGraphArrivals->attachAxis(xAxisMonth);
	monthGraphArrivals->attachAxis(yAxisMonth);
	monthGraphDepartures->attachAxis(xAxisMonth);
	monthGraphDepartures->attachAxis(yAxisMonth);

	monthChartView->show();

	semaphore.release(1);
}

//слот приема статистики за год
void Statistic::receiveStatisticYear(QVector<QPair<QDate, double>> *arrivalsCount, QVector<QPair<QDate, double>> *departuresCount)
{
	if(counterStatisticYear > EARLY_EXIT_COUNT)
	{
		semaphore.release(1);
		return;
	}
	if(yearChart->series().isEmpty() == false)
	{		
		yearChart->removeSeries(yearGraph);
		yearChart->removeAxis(xAxisYear);
		yearChart->removeAxis(yAxisYear);
		xAxisYear->clear();
	}
	if(yearGraph->count() == 0)
	{
		yearGraph->append(arrivalsSet);
		yearGraph->append(departuresSet);
	}
	arrivalsSet->remove(0, arrivalsSet->count());
	departuresSet->remove(0, departuresSet->count());


	//для настройки оси Y будем фиксировать максимальное значение графика
	double maxArrivals = 0.0;
	double maxDepartures = 0.0;

	//график прилетов
	for (auto it = arrivalsCount->constBegin(); it != arrivalsCount->constEnd(); ++it)
	{
		if(counterStatisticYear > EARLY_EXIT_COUNT)
		{
			semaphore.release(1);
			return;
		}
		arrivalsSet->append(it->second);
		xAxisYear->append(intToMonthRus(it->first.month()) + "-" + QString::number(it->first.year()));
		maxArrivals = maxArrivals < it->second ? it->second : maxArrivals;
	}

	//график вылетов
	for (auto it = departuresCount->constBegin(); it != departuresCount->constEnd(); ++it)
	{
		if(counterStatisticYear > EARLY_EXIT_COUNT)
		{
			semaphore.release(1);
			return;
		}
		departuresSet->append(it->second);
		xAxisYear->append(intToMonthRus(it->first.month()) + "-" + QString::number(it->first.year()));
		maxDepartures = maxDepartures < it->second ? it->second : maxDepartures;
	}

	//настраиваем график и выводим на экран
	yearChart->addSeries(yearGraph);

	yAxisYear->setRange(0, 1.1*(maxArrivals + maxDepartures));
	yAxisYear->setLabelFormat("%d");
	xAxisYear->setLabelsAngle(90);

	yearChart->addAxis(xAxisYear, Qt::AlignBottom);
	yearChart->addAxis(yAxisYear, Qt::AlignLeft);
	yearGraph->attachAxis(xAxisYear);
	yearGraph->attachAxis(yAxisYear);

	yearChartView->show();

	semaphore.release(1);
}

//СЛОТЫ WIDGET-ОВ ОКНА

//слот запроса статитстики по году выбранному в QComboBox cb_year_1
void Statistic::on_cb_year_1_activated(int index)
{
	int year = yearsModel->data(yearsModel->index(index, 0)).toInt();

	++counterStatisticYear;
	if(counterStatisticYear > MAX_THREADS_COUNT){ui->centralwidget->setEnabled(false);}
	auto f = QtConcurrent::run([&, year]()
	{
		semaphore.acquire(1);

		QDate from = std::min(airportDB->getMinDateArrival(), airportDB->getMinDateDeparture());
		QDate to = std::max(airportDB->getMaxDateArrival(), airportDB->getMaxDateDeparture());

		//исключаем неполные месяцы из статистики
		truncDays(from, to);

		if(from.year() < year)
		{
			from.setDate(year, 1, 1);
		}

		if(to.year() > year)
		{
			to.setDate(year, 12, 31);
		}

		airportDB->getStatisticYear(from, to, airportCode);

		--counterStatisticYear;
		if(counterStatisticYear < MIN_THREADS_COUNT){ui->centralwidget->setEnabled(true);}
	});
}

//вывод в QComboBox cb_month_2 месяцев в году выбранном в cb_year_2
void Statistic::on_cb_year_2_activated(int index)
{
	qDebug()<<__FUNCTION__;
	ui->cb_month_2->setEnabled(false);
	int year = yearsModel->data(yearsModel->index(index, 0)).toInt();
	++counterMonthsModel;
	if(counterMonthsModel > MAX_THREADS_COUNT){ui->centralwidget->setEnabled(false);}
	auto f = QtConcurrent::run([&, year]()
	{		
		qDebug()<<__FUNCTION__;
		qDebug() << year;
		semaphore.acquire(1);

		airportDB->getMonths(year);

		--counterMonthsModel;
		if(counterMonthsModel < MIN_THREADS_COUNT){ui->centralwidget->setEnabled(true);}
	});
}

//слот запроса статистики при выборе месяца в QComboBox cb_month_2
void Statistic::on_cb_month_2_activated(int index)
{
	++counterStatisticMonth;
	if(counterStatisticMonth > MAX_THREADS_COUNT){ui->centralwidget->setEnabled(false);}
	auto f = QtConcurrent::run([&, index]()
	{
		semaphore.acquire(1);

		int month = MonthRusToInt(ui->cb_month_2->currentText());
		int year = yearsModel->data(yearsModel->index(ui->cb_year_2->currentIndex(), 0)).toInt();
		QDate from;
		QDate to;
		from.setDate(year, month, 1);
		to.setDate(year, month, 1);
		to.setDate(year, month, to.daysInMonth());

		airportDB->getStatisticMonth(from, to, airportCode);

		--counterStatisticMonth;
		if(counterStatisticMonth < MIN_THREADS_COUNT){ui->centralwidget->setEnabled(true);}
	});
}

//слот запроса статистики при выборе вкладки
void Statistic::receiveTabIndex(int index)
{
	if(index == 0)
	{
		on_cb_all_years_toggled(ui->cb_all_years->isChecked());
	}
	else if(index == 1)
	{
		on_cb_month_2_activated(ui->cb_month_2->currentIndex());
	}
}

//отображение статистики по всем годам сразу или по году выбранному в ComboBox cb_year_1
void Statistic::on_cb_all_years_toggled(bool checked)
{
	if(checked)
	{
		ui->cb_year_1->setEnabled(false);
		++counterYearsModel;
		if(counterYearsModel > MAX_THREADS_COUNT){ui->centralwidget->setEnabled(false);}
		auto f = QtConcurrent::run([&]()
		{

			semaphore.acquire(1);

			QDate from = std::min(airportDB->getMinDateArrival(), airportDB->getMinDateDeparture());
			QDate to = std::max(airportDB->getMaxDateArrival(), airportDB->getMaxDateDeparture());
			//исключаем неполные месяцы из статистики
			truncDays(from, to);

			airportDB->getStatisticYear(from, to, airportCode);

			--counterYearsModel;
			if(counterYearsModel < MIN_THREADS_COUNT){ui->centralwidget->setEnabled(true);}
		});

	}
	else
	{
		ui->cb_year_1->setEnabled(true);
		on_cb_year_1_activated(ui->cb_year_1->currentIndex());
	}

}


//	ВСПОМОГАТЕЛЬНЫЕ PRIVATE ФУНКЦИИ
////////////////////////////////////////////////////////////////////////////////////////////

//исключаем неполные месяцы из статистики
//from принимает ближайшее первое число следующего месяца, если текущий месяц неполный
//to принимает ближайшее последнее число предшествующего месяца,если текущий месяц неполный
void Statistic::truncDays(QDate &from, QDate &to)
{
	if(from.day() > 1)
	{
		if(from.month() == 12)
		{
			from.setDate(from.year() + 1, 1, 1);
		}
		else
		{
			from.setDate(from.year(), from.month() + 1, 1);
		}
	}
	if(to.day() < to.daysInMonth())
	{
		if(to.month() == 1)
		{
			to.setDate(to.year() - 1, 12, 1);
			to.setDate(to.year(), to.month(), to.daysInMonth());
		}
		else
		{
			to.setDate(to.year(), to.month() - 1, 1);
			to.setDate(to.year(), to.month(), to.daysInMonth());
		}
	}
}

//Получаем назание месяца на русском, стандартные функции дают неверный падеж
QString Statistic::intToMonthRus(int month)
{
	switch (month)
	{
	case 1:
		return "Январь";
	case 2:
		return "Февраль";
	case 3:
		return "Март";
	case 4:
		return "Апрель";
	case 5:
		return "Май";
	case 6:
		return "Июнь";
	case 7:
		return "Июль";
	case 8:
		return "Август";
	case 9:
		return "Сентябрь";
	case 10:
		return "Октябрь";
	case 11:
		return "Ноябрь";
	case 12:
		return "Декабрь";
	default:
		return "";
	}
}

//Получаем номер месяца на основании русского наименования
int Statistic::MonthRusToInt(QString month)
{
	if("Январь" == month){return 1;}
	if("Февраль" == month){return 2;}
	if("Март" == month){return 3;}
	if("Апрель" == month){return 4;}
	if("Май" == month){return 5;}
	if("Июнь" == month){return 6;}
	if("Июль" == month){return 7;}
	if("Август" == month){return 8;}
	if("Сентябрь" == month){return 9;}
	if("Октябрь" == month){return 10;}
	if("Ноябрь" == month){return 11;}
	if("Декабрь" == month){return 12;}
	return 0;
}
