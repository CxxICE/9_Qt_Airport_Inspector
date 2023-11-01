#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	disconnectionActivated = false;
	msgBoxClosed = false;
	setWindowIcon(QIcon(":/img/airplane.ico"));
	ui->centralwidget->setEnabled(false);
	counter = 0;

	//установка фильтра против исчезновения статуса при наведении мыши на меню
	ui->menubar->installEventFilter(this);

	//настройка StatuBar
	lb_statusLabel = new QLabel(this);
	ui->statusbar->addPermanentWidget(lb_statusLabel);
	lb_statusLabel->setPixmap((QPixmap(":/img/NoColor.jpg", "JPG", Qt::ColorOnly)));

	//вспомогательные модели для MainWindow
	mapper = new QDataWidgetMapper(this);
	itemModel = new QStandardItemModel(this);
	items =nullptr;

	//окно ввода данных для подключения БД
	dialogSQL = new LoginDialogSql(this);

	//окно вывода ошибок
	msgBox = new QMessageBox(this);

	//данные для подключения в БД
	airportDB = new AirportDB(this);
	airportDB->addDB(POSTGRE_DRIVER, DB_NAME);
	connectionData = dialogSQL->getConnectionData();

	//окно диаграмм со статистикой
	statisticWindow = new Statistic(airportDB, this);

	//проброс данных из формы настроек БД в главную форму
	connect(dialogSQL, &LoginDialogSql::sig_sendConnectionData, this, [&](DBConnectionData dbData){
		connectionData = dbData;
	});

	//прием сигнала ошибки из БД
	connect(airportDB, &AirportDB::sig_SendError, this, &MainWindow::receiveError);
	//прием сигнала закрытия MsgBox
	connect(msgBox, &QMessageBox::finished, this, &MainWindow::receiveFinishedMsg);
	//установка статуса подключения
	connect(airportDB, &AirportDB::sig_SendStatusConnection, this, &MainWindow::receiveStatusConnection);
	//принимаем результат запроса по списку аэропортов
	connect(airportDB, &AirportDB::sig_SendAirportsModel, this, &MainWindow::receiveAirportsModel);
	//принимаем результат запроса по перелетам
	connect(airportDB, &AirportDB::sig_SendFlightsModel, this, &MainWindow::receiveFlightsModel);

	//запуск подключения БД и настройки MainWindow по данным из БД
	auto f = QtConcurrent::run([&]()
	{		
		statisticWindow->acquireSemaphore(1);
		initializeDB();

	});	
}

MainWindow::~MainWindow()
{	
	resetItemModel();
	delete ui;
}

//освобождение items из itemModel и удаление памяти выделенной для items с иcпользованиме placement new
void MainWindow::resetItemModel()
{
	//освобождаем items из владения itemModel, необходимо в связи с применением placement new
	for (int y = 0; y < itemModel->rowCount(); ++y)
	{
		for (int x = 0; x < itemModel->columnCount(); ++x)
		{
			itemModel->takeItem(y,x);
		}
	}
	itemModel->setColumnCount(0);
	itemModel->setRowCount(0);

	if(items != nullptr)
	{
		delete items;
	}
}

//фильтр против исчезновения статуса при наведении мыши на меню
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
	if(watched == ui->menubar && event->type() == QEvent::StatusTip)
	{
		return true;
	}
	else
	{
		return watched->eventFilter(watched,event);
	}
}

//метод по подключению БД и загрузки из нее необходимых для MainWindow данных:
//названия и коды аэропортов
//ограничения на допустимые даты прилета и вылета
void MainWindow::initializeDB()
{
	disconnectionActivated = false;//если пользователь выберет в меню "Отключиться от БД" в процессе подключения -> прждевременный выход
	bool repeat = false;//флаг повторного подключения, если первая попытка не удалась
	//подключаемся к БД пока пользователь не нажмет отключиться или не произойдет успешное подключение
	do
	{
		ui->statusbar->setStyleSheet("color:black");
		lb_statusLabel->setPixmap((QPixmap(":/img/Yellow.jpg", "JPG", Qt::ColorOnly)));

		if(repeat)//если это не первая попытка подключения
		{
			statisticWindow->acquireSemaphore(1);
			//тормозим поток, пока пользователь не закроет сообщение об ошибке подключения
			while(!msgBoxClosed)
			{
				QThread::yieldCurrentThread();
			}
			//отображаем секунды до повторного подключения
			QString statusMsg = "Повторное подключение через %1 сек";
			for(int i = 5; i >= 1; --i)
			{
				if(disconnectionActivated == true)
				{
					break;
				}
				ui->statusbar->showMessage(statusMsg.arg(i));
				QThread::sleep(1);
			}
			ui->statusbar->showMessage(statusMsg.arg(0));
		}
		else//первая попытка подключения
		{
			if(disconnectionActivated == true)
			{
				break;
			}
			ui->statusbar->showMessage("Подключение...");
		}
		//попытка подключения
		airportDB->connectDB(connectionData);
		repeat = true;

	} while (!(airportDB->isOpen()) && disconnectionActivated == false);
	//
	if(!(airportDB->isOpen()))//неуспешное подключение
	{
		receiveStatusConnection(false);		
	}
	else//успешное подключение
	{
		//получаем список аэропортов из БД
		airportDB->getAirports();
		//установка минимальной и максимальной дат на основе дат присутствующих в БД
		if(ui->rb_departure->isChecked())
		{
			ui->de_from_date->setMinimumDate(airportDB->getMinDateDeparture());
			ui->de_from_date->setMaximumDate(airportDB->getMaxDateDeparture());
			ui->de_to_date->setMinimumDate(airportDB->getMinDateDeparture());
			ui->de_to_date->setMaximumDate(airportDB->getMaxDateDeparture());
			ui->de_from_date->setDate(ui->de_from_date->minimumDate());
			ui->de_to_date->setDate(ui->de_to_date->maximumDate());
		}
		else if(ui->rb_arrival->isChecked())
		{
			ui->de_from_date->setMinimumDate(airportDB->getMinDateArrival());
			ui->de_from_date->setMaximumDate(airportDB->getMaxDateArrival());
			ui->de_to_date->setMinimumDate(airportDB->getMinDateArrival());
			ui->de_to_date->setMaximumDate(airportDB->getMaxDateArrival());
			ui->de_from_date->setDate(ui->de_from_date->minimumDate());
			ui->de_to_date->setDate(ui->de_to_date->maximumDate());
		}
		ui->centralwidget->setEnabled(true);
	}
	return;
}

//слот отображения ошибок
void MainWindow::receiveError(QString error)
{
	msgBoxClosed = false;
	msgBox->setIcon(QMessageBox::Critical);
	msgBox->setText(error);
	msgBox->exec();	
	if(statisticWindow->availableSemaphores() == 0)
	{
		statisticWindow->releaseSemaphore(1);
	}
}

//слот отображения статуса подключения БД
void MainWindow::receiveStatusConnection(bool status)
{
	if(status)
	{
		ui->statusbar->setStyleSheet("color:green");
		ui->statusbar->showMessage("Подключено");
		lb_statusLabel->setPixmap((QPixmap(":/img/Green.jpg", "JPG", Qt::ColorOnly)));
	}
	else
	{
		ui->statusbar->setStyleSheet("color:red");
		ui->statusbar->showMessage("Отключено");
		lb_statusLabel->setPixmap((QPixmap(":/img/Red.jpg", "JPG", Qt::ColorOnly)));
		disconnectionActivated = true;
		ui->centralwidget->setEnabled(false);		
	}	
}

//слот маппирования аэропортов на combobox и qlabel
void MainWindow::receiveAirportsModel(QSqlQueryModel *airportsModel)
{
	ui->cb_airports->setModel(airportsModel);
	ui->cb_airports->setModelColumn(0);
	mapper->setModel(airportsModel);
	mapper->addMapping(ui->lb_airport_code, 1, "text");
	mapper->setCurrentIndex(ui->cb_airports->currentIndex());	
	statisticWindow->releaseSemaphore(1);
}

//слот маппирования данных о перелетах на tableview
void MainWindow::receiveFlightsModel(QSqlQueryModel *flightsModel)
{
	if(counter > 1)
	{
		statisticWindow->releaseSemaphore(1);
		return;
	}
	ui->tv_flights->setModel(nullptr);
	resetItemModel();

	//выделение памяти с placement new
	int bufer_size = sizeof(QStandardItem)*(flightsModel->rowCount())*(flightsModel->columnCount());
	items = new char[bufer_size];

	//выравнивание по центру ячейки и обеспечение сортировки через QStandardItemModel
	for (int y = 0; y < flightsModel->rowCount(); ++y)
	{
		for (int x = 0; x < flightsModel->columnCount(); ++x)
		{			
			if(counter > 1)
			{
				statisticWindow->releaseSemaphore(1);
				return;
			}
			//размещение item с placement new
			QStandardItem *item = new (items + (flightsModel->columnCount()*y + x)*sizeof(QStandardItem))QStandardItem;

			if(x == 1)
			{
				item->setText(flightsModel->index(y,x).data().toDateTime().toString("yyyy.MM.dd hh:mm:ss"));
			}
			else
			{
				item->setText(flightsModel->index(y,x).data().toString());
			}
			item->setTextAlignment(Qt::AlignCenter);
			itemModel->setItem(y,x,item);//берет item во владение
		}
	}

	//настройка модели отображения перелетов и подстройка геометрии окна
	ui->tv_flights->setModel(itemModel);
	itemModel->setHeaderData(0,Qt::Horizontal, tr("Номер рейса"));
	if(ui->rb_departure->isChecked())
	{
		itemModel->setHeaderData(1,Qt::Horizontal, tr("Время вылета"));
		itemModel->setHeaderData(2,Qt::Horizontal, tr("Аэропорт назначения"));
	}
	else if(ui->rb_arrival->isChecked())
	{
		itemModel->setHeaderData(1,Qt::Horizontal, tr("Время прилета"));
		itemModel->setHeaderData(2,Qt::Horizontal, tr("Аэропорт отправления"));
	}
	ui->tv_flights->verticalHeader()->setVisible(false);
	ui->tv_flights->setSortingEnabled(true);
	ui->tv_flights->sortByColumn(1,Qt::SortOrder::AscendingOrder);

	auto oldTableSize = ui->tv_flights->width();
	auto oldWinSize = this->width();
	ui->tv_flights->resizeColumnsToContents();
	ui->tv_flights->setColumnWidth(0,ui->tv_flights->columnWidth(0) + 50);
	ui->tv_flights->setColumnWidth(1,ui->tv_flights->columnWidth(1) + 50);
	ui->tv_flights->setColumnWidth(2,ui->tv_flights->columnWidth(2) + 50);
	auto newTableSize = ui->tv_flights->columnWidth(0) + ui->tv_flights->columnWidth(1) + ui->tv_flights->columnWidth(2);

	this->resize(oldWinSize + newTableSize - oldTableSize + 20, this->height());
	ui->tv_flights->scrollToTop();
	if(counter < MIN_THREADS_COUNT)
	{
		lb_statusLabel->setPixmap((QPixmap(":/img/Green.jpg", "JPG", Qt::ColorOnly)));
	}

	statisticWindow->releaseSemaphore(1);
}



//СЛОТЫ RADIOBUTTON
//динамическая установка диапазона допустимых дат для вылетов
void MainWindow::on_rb_departure_clicked(bool checked)
{
	if(checked && airportDB->isOpen())
	{
		auto f = QtConcurrent::run([&]()
		{
			++counter;
			if(counter > MAX_THREADS_COUNT){
				ui->centralwidget->setEnabled(false);
				lb_statusLabel->setPixmap((QPixmap(":/img/Yellow.jpg", "JPG", Qt::ColorOnly)));
			}
			statisticWindow->acquireSemaphore(1);

			ui->de_from_date->setMinimumDate(airportDB->getMinDateDeparture());
			ui->de_from_date->setMaximumDate(airportDB->getMaxDateDeparture());
			ui->de_to_date->setMinimumDate(airportDB->getMinDateDeparture());
			ui->de_to_date->setMaximumDate(airportDB->getMaxDateDeparture());

			--counter;
			if(counter < MIN_THREADS_COUNT){
				ui->centralwidget->setEnabled(true);
				lb_statusLabel->setPixmap((QPixmap(":/img/Green.jpg", "JPG", Qt::ColorOnly)));
			}
			statisticWindow->releaseSemaphore(1);
		});
	}
	return;
}

//динамическая установка диапазона допустимы дат для прилетов
void MainWindow::on_rb_arrival_clicked(bool checked)
{
	if(checked && airportDB->isOpen())
	{
		auto f = QtConcurrent::run([&]()
		{
			++counter;
			if(counter > MAX_THREADS_COUNT){
				ui->centralwidget->setEnabled(false);
				lb_statusLabel->setPixmap((QPixmap(":/img/Yellow.jpg", "JPG", Qt::ColorOnly)));
			}
			statisticWindow->acquireSemaphore(1);

			ui->de_from_date->setMinimumDate(airportDB->getMinDateArrival());
			ui->de_from_date->setMaximumDate(airportDB->getMaxDateArrival());
			ui->de_to_date->setMinimumDate(airportDB->getMinDateArrival());
			ui->de_to_date->setMaximumDate(airportDB->getMaxDateArrival());

			--counter;
			if(counter < MIN_THREADS_COUNT){
				ui->centralwidget->setEnabled(true);
				lb_statusLabel->setPixmap((QPixmap(":/img/Green.jpg", "JPG", Qt::ColorOnly)));
			}
			statisticWindow->releaseSemaphore(1);
		});
	}
	return;
}

//динамическое отображение кода аэропорта в QLabel при выборе названия аэропорта в ComboBox cb_airports
void MainWindow::on_cb_airports_currentIndexChanged(int index)
{
	mapper->setCurrentIndex(index);
}

//слот выполняемый по закрытию msgBox
void MainWindow::receiveFinishedMsg(int)
{
	msgBoxClosed = true;//отпираем поток initializeDB
}


//СЛОТЫ PUSHBUTTONS

//запрос данных по прилетам и вылетам
void MainWindow::on_pb_request_clicked()
{
	this->lb_statusLabel->setPixmap((QPixmap(":/img/Yellow.jpg", "JPG", Qt::ColorOnly)));
	if(ui->rb_departure->isChecked())
	{
		auto f = QtConcurrent::run([&]()
		{
			++counter;
			if(counter > MAX_THREADS_COUNT){
				ui->centralwidget->setEnabled(false);
				lb_statusLabel->setPixmap((QPixmap(":/img/Yellow.jpg", "JPG", Qt::ColorOnly)));
			}
			statisticWindow->acquireSemaphore(1);

			airportDB->getDepartures(ui->de_from_date->date(), ui->de_to_date->date(), ui->lb_airport_code->text());

			--counter;
			if(counter < MIN_THREADS_COUNT){
				ui->centralwidget->setEnabled(true);
				lb_statusLabel->setPixmap((QPixmap(":/img/Green.jpg", "JPG", Qt::ColorOnly)));
			}
		});
	}
	else if(ui->rb_arrival->isChecked())
	{
		auto f = QtConcurrent::run([&]()
		{
			++counter;
			if(counter > MAX_THREADS_COUNT){
				ui->centralwidget->setEnabled(false);
				lb_statusLabel->setPixmap((QPixmap(":/img/Yellow.jpg", "JPG", Qt::ColorOnly)));
			}
			statisticWindow->acquireSemaphore(1);

			airportDB->getArrivals(ui->de_from_date->date(), ui->de_to_date->date(), ui->lb_airport_code->text());

			--counter;
			if(counter < MIN_THREADS_COUNT){
				ui->centralwidget->setEnabled(true);
				lb_statusLabel->setPixmap((QPixmap(":/img/Green.jpg", "JPG", Qt::ColorOnly)));
			}
		});
	}
	return;
}

//передача информации об аэропорте в StatisticWindow и его открытие
void MainWindow::on_pb_statistic_clicked()
{
	statisticWindow->setWindowModality(Qt::WindowModal);//для блокировка MainWindow
	statisticWindow->setStatistic(ui->cb_airports->currentText(), ui->lb_airport_code->text());
	statisticWindow->show();
}

//очистка таблицы
void MainWindow::on_pb_clear_clicked()
{
	ui->tv_flights->setModel(nullptr);
}


//СЛОТЫ ДЛЯ КНОПОК МЕНЮ
//меню->выход
void MainWindow::on_mb_exit_triggered()
{
	this->close();
}

//меню->настройка БД
void MainWindow::on_mb_db_settings_triggered()
{
	dialogSQL->exec();
}

//меню->подключения/отключение БД
void MainWindow::on_mb_db_connection_triggered()
{
	if (ui->mb_db_connection->text() == "Отключиться от БД")
	{
		ui->mb_db_connection->setText("Подключиться к БД");
		airportDB->disconnectDB();
	}
	else
	{
		ui->mb_db_connection->setText("Отключиться от БД");
		auto f = QtConcurrent::run([&]()
		{			
			statisticWindow->acquireSemaphore(1);
			initializeDB();			
		});
	}
}
