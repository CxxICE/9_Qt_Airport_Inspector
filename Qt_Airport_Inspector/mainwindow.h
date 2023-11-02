#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QScopedPointer>
#include <QThread>
#include <QDataWidgetMapper>
#include <QStandardItemModel>
#include <QtConcurrent>
#include <QSemaphore>
#include <QAtomicInt>

#include "statistic.h"
#include "logindialogsql.h"
#include "airportdb.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void receiveError(QString error);
	void receiveStatusConnection(bool status);

	void receiveAirportsModel(QSqlQueryModel *airportsModel);
	void receiveFlightsModel(QSqlQueryModel *flightsModel);

	//слоты для QPushButton
	//1 - открытие окне статистики для выбранного аэропорта
	//2 - запрос таблицы вылетов/прилетов из БД по введенным данным
	//3 - очистка QTextEdit
	void on_pb_statistic_clicked();
	void on_pb_request_clicked();
	void on_pb_clear_clicked();

	//слоты для кнопок главного меню
	//1 - отключение/подключение БД
	//2 - настройка данных для подключения к БД
	//3 - выход из программы
	void on_mb_db_connection_triggered();
	void on_mb_db_settings_triggered();
	void on_mb_exit_triggered();

	//слоты для автоматической установки ограничений на вводимые даты в QDateEdit при выборе пользователем вылетов или прилетов
	void on_rb_departure_clicked(bool checked);
	void on_rb_arrival_clicked(bool checked);

	//слот обработки выбранного пользоателем аэропорта, для вывода кода аэропорта в QLabel
	void on_cb_airports_currentIndexChanged(int index);

	//слот принимает сигнал о закрытии окна сообщения об ошибке QMessageBox *msgBox для старта 5 секунд - повторного подключения
	void receiveFinishedMsg(int);



private:
	Ui::MainWindow			*ui;

	QMessageBox				*msgBox;
	LoginDialogSql			*dialogSQL;
	Statistic				*statisticWindow;	

	//база данных и данные для подключения
	AirportDB				*airportDB;
	DBConnectionData		connectionData;

	//Индикатор-светофор в StatuBar
	//Красный - отключено
	//желтый - в процессе подключения или занят запросом к БД
	//зеленый - подключен, пользователь может вводить данные для запроса
	QLabel					*lb_statusLabel;

	//модель с перечнем аэропортов для QComboBox cb_airports
	QSqlQueryModel			*airportsModel;

	//модель для QTextEdit, применена отдельная модель для возможности сортировки и выравнивания данных по центру ячейки
	QStandardItemModel		*itemModel;
	char					*items;

	//mapper - для маппирования кода аэропорта на QLabel lb_airport_code
	QDataWidgetMapper		*mapper;

	bool					disconnectionActivated;
	bool					msgBoxClosed;

	//счетчик созданных потоков в QtConcurrent --> при превышении макисмального числа потоков виджет блокирукется,
	//пока число потоков не снизится до минимального
	QAtomicInt counterRadioButton;
	QAtomicInt counterFlightsModel;

	//метод по подключению БД и загрузки из нее необходимых для MainWindow данных:
	//названия и коды аэропортов
	//ограничения на допустимые даты прилета и вылета
	void initializeDB();

	//освобождение items из itemModel и удаление памяти выделенной для items с иcпользованиме placement new
	void resetItemModel();

protected:
	//фильтр для устранения исчезносения статуса подключения БД при наведении мыши на меню
	bool eventFilter(QObject *watched, QEvent *event);
};
#endif // MAINWINDOW_H
