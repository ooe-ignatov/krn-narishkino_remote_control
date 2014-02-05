#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtGui>
#include <QMessageBox>
#include <QTextCodec>
#include <QList>
#include <QBrush>
#include <QListWidgetItem>
#include <QTimer>
#include <QProcess>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();



private slots:
    void initialapp(); // чтение настроек
    void set_address(); // запись настроек ip
    void set_port (); // запись настроек порта
    void connect_to_moxa(); // подключение к moxa для опроса
    void connect_for_on(); // подключения для включения
    void connect_for_off(); // подключения для включения
    void ask_dio_status(); // запрос состояний DIO
    void getState(); // получает информацию о состоянии
    void showState (); //размещает в окне
    void turnOn (); // включает крн
    void turnOff (); // выключает крн
    void reset (); // Сбрасывает управляемые каналы в 0
    void bad_connection(); // Выводит сообщение о слишком долгом выполнении команды и закрывает соединения


private:
    QSettings krn_settings; // настройки приложения

    int ask_timer; // значение таймера опроса, при ошибке подключения будет опрашиваться чаще
    QTimer *timer; // сам таймер опроса
    QTimer *control_timer; // Таймер для контроля управления КРН, если после отправки команды пройдет слишком много времени, то будет выведено соответсвующее сообщение
    QList<QString> res_dio; // ответ от MOXA
    QString state_old; // Состояние КРН, с которым будем сравнивать
    QListWidgetItem *element; // Используется для установки в объекте QListWidget]
    QString ip; // адрес подключения
    quint16 port; // порт подключения
    QTcpSocket *sock; // сокет для опроса
    QTcpSocket *on_sock; // сокет для включения
    QTcpSocket *off_sock; // сокет для отключения

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
