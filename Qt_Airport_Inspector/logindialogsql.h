#ifndef LOGINDIALOGSQL_H
#define LOGINDIALOGSQL_H

#include <QDialog>

#include "structs.h"

//Форма ввода и хранения данных для кодключения к БД

namespace Ui {
class LoginDialogSql;
}

class LoginDialogSql : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialogSql(QWidget *parent = nullptr);
    ~LoginDialogSql();
	DBConnectionData getConnectionData();

signals:
	void sig_sendConnectionData(DBConnectionData dbData);

private slots:

    void on_pb_Cancel_clicked();
	void on_pb_Ok_clicked();

private:
    Ui::LoginDialogSql *ui;
	DBConnectionData connectionData;
};

#endif // LOGINDIALOGSQL_H
