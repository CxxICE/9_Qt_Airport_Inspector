#include "logindialogsql.h"
#include "ui_logindialogsql.h"


LoginDialogSql::LoginDialogSql(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialogSql)
{
    ui->setupUi(this);
	ui->le_password->setEchoMode(QLineEdit::EchoMode::Password);
	setWindowIcon(QIcon(":/img/airplane.ico"));

	//значения по умолчанию
	ui->le_host->setText("981757-ca08998.tmweb.ru");
	ui->sb_port->setValue(5432);
	ui->le_dbname->setText("demo");
	ui->le_username->setText("netology_usr_cpp");
	ui->le_password->setText("CppNeto3");
	this->on_pb_Ok_clicked();
}

LoginDialogSql::~LoginDialogSql()
{
	delete ui;
}

DBConnectionData LoginDialogSql::getConnectionData()
{
	return connectionData;
}

void LoginDialogSql::on_pb_Ok_clicked()
{
	connectionData.host = ui->le_host->text();
	connectionData.port = ui->sb_port->value();
	connectionData.dbName = ui->le_dbname->text();
	connectionData.userName = ui->le_username->text();
	connectionData.password = ui->le_password->text();
	emit sig_sendConnectionData(connectionData);
	this->close();
    return;
}

void LoginDialogSql::on_pb_Cancel_clicked()
{
    this->close();
    return;
}

