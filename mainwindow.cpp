#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qtcpsocket.h>
#include <QString>
#include <QLineEdit>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    krn_settings("OOE", "krn_narishkino"),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->move(QApplication::desktop()->availableGeometry().topLeft()); // запуск окна в левом верхнем углу
    //создаем сокет
    sock = new QTcpSocket(this); // сокет для опроса
    on_sock = new QTcpSocket(this); // сокет для включения
    off_sock = new QTcpSocket(this); // сокет для отключения

    ui->ip_label->setText(ip);
    //соединяем сигналы и прочее
    state_old=""; // обнулили состояние
    ask_timer=60000;
    timer = new QTimer; // таймер по которому будет копироваться файл и соответственно запускаться его сравнение
    control_timer = new QTimer;
    connect(timer, SIGNAL(timeout()), this, SLOT(connect_to_moxa())); // соединение сигнала таймера и считывания DIO
    connect(ui->Refresh_but, SIGNAL(clicked()), this, SLOT(connect_to_moxa())); // Обновление по нажатию кнопки
    connect(sock, SIGNAL(connected()), this, SLOT(ask_dio_status()));
    connect(sock, SIGNAL(readyRead()), this, SLOT(getState()));
    connect(ui->onBut, SIGNAL(clicked()), this, SLOT(connect_for_on()));
    connect(ui->offBut, SIGNAL(clicked()), this, SLOT(connect_for_off()));
    connect(on_sock, SIGNAL(connected()), this, SLOT(turnOn()));
    connect(off_sock, SIGNAL(connected()), this, SLOT(turnOff()));
    connect(on_sock, SIGNAL(readyRead()), this, SLOT(reset()));
    connect(off_sock, SIGNAL(readyRead()), this, SLOT(reset()));
    connect(control_timer, SIGNAL(timeout()), this, SLOT(bad_connection()));
    connect(ui->Address, SIGNAL(triggered()), this, SLOT(set_address()));
    connect(ui->Port, SIGNAL(triggered()), this, SLOT(set_port()));

    initialapp();

}

/********************** Инициализация приложения *************************/
void MainWindow::initialapp()
{
    if ((!krn_settings.value("/settings/address").isNull()) && (!krn_settings.value("/settings/port").isNull()))
    {
        ip = krn_settings.value("/settings/address").toString();
        port = krn_settings.value("/settings/port").toString().toShort();
        ui->ip_label->setText(ip);
        connect_to_moxa();
    }
    else
    {
        ip = "Нет настроек подключения!";
        ui->ip_label->setText(ip);
    }
}

/********************* Запись настроек IP *********************/
void MainWindow::set_address()
{
    bool set_ip;
    QString str = QInputDialog::getText(this, "Введите адрес", "Адрес:", QLineEdit::Normal, "", &set_ip);

    if (!set_ip)
    {
        return;
    }
    else
    {
        krn_settings.setValue("/settings/address", str);
        initialapp();
    }
}

/**************** Запись настроек порта *******************/
void MainWindow::set_port()
{
    bool set_port;
    QString str = QInputDialog::getText(this, "Введите порт", "Порт:", QLineEdit::Normal, "", &set_port);

    if (!set_port)
    {
        return;
    }
    else
    {
        krn_settings.setValue("/settings/port", str);
        initialapp();
    }
}

/************************************** Подключение к мохе *******************************/
void MainWindow::connect_to_moxa()
{
    timer->start(ask_timer); //запуск таймера
    sock->connectToHost(ip, port); // Соединяемся с указанным адресом
    if (!sock->waitForConnected(10000)) // ели за 10 секунд сервер нас не пустит, то выдаем ошибку.
    {
        ask_timer=15000; // меняем частоту опроса

        QTextCodec* codec = QTextCodec::codecForName("UTF8"); // указываем кодировку
        QTextCodec::setCodecForTr(codec);
        QTextCodec::setCodecForCStrings(codec);
        QTextCodec::setCodecForLocale(codec);
        QBrush blue = QBrush(QColor(100, 100, 255), Qt::SolidPattern); // Кисть синего цвета

        ui->Statewgt -> clear(); // Очистка области под флажки.
        ui->Statewgt -> setResizeMode(QListView::Adjust);
        ui->Statewgt -> setMovement(QListView::Static); // Указываем, что элементы не могут быть перемещены пользователем.
        ui->Statewgt -> setSpacing(8);
        ui->Statewgt -> setFlow(QListView::LeftToRight); // Устанавливаем порядок расположения элементов слева на право.
        ui->Statewgt -> setWrapping(true); // Устанавливаем свойство, при  котором элементы переносятся, если выходят за область отображения.

        element = new QListWidgetItem("", ui->Statewgt); // Создаем новый экземпляр для элемента объекта QListWidget, указывая этот объект аргументом в качестве родителя.
        element -> setSizeHint(QSize(95,24));

        element->setBackground(blue);
        element -> setText("Нет связи!");

        qDebug() << "Нет связи!";

        if (state_old!="err")
        {
        QProcess beep;
        QString bep = "beep -f 2000 -n -f 1500 -n -f 1000";
        beep.start(bep);
        beep.waitForFinished(1000);
        state_old="err";
        }
    }
}


/******************************** ОТПРАВКА ОПРОСА MOXA *******************************/
void MainWindow::ask_dio_status ()
{
    QByteArray ask_dio; // команда запроса состояний

    ask_dio.resize(6); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100
    ask_dio[0]=05;
    ask_dio[1]=02;
    ask_dio[2]=00;
    ask_dio[3]=02;
    ask_dio[4]=00;
    ask_dio[5]=03;

    sock->write(ask_dio); // отправка запроса
}


/**************************** ПОЛУЧЕНИЕ ОТВЕТА **************************/
void MainWindow::getState()
{
    res_dio.clear(); // очищаем результат ответа

    int i=sock->bytesAvailable(); // количество байт, которое будет принято
    int j; // счетчик
    for (j=0; j<i; j++)
    {
        res_dio << sock->read(1).toHex(); // Пишем в список побайтно ответ от MOXA
    }
    sock->close(); // закрываем соединение

    qDebug() << "Multiple DIO Status: " << res_dio;

    // если состояние изменилось, то рисуем это в окне
    if (res_dio.at(5)!=state_old)
    {
    showState (); // Запускаем формирование окна состояний
    }
}


/******************** ФОРМИРОВАНИЕ ОКНА СОСТОЯНИЙ *******************/
void MainWindow::showState ()
{
    QTextCodec* codec = QTextCodec::codecForName("UTF8"); // указываем кодировку
    QTextCodec::setCodecForTr(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForLocale(codec);
    QBrush green = QBrush(QColor(100, 255, 100), Qt::SolidPattern); // Кисть зеленого цвета
    QBrush red = QBrush(QColor(255, 100, 100), Qt::SolidPattern); // Кисть красного цвета

    //ui->ip_label->setText(ip);
    ui->Statewgt -> clear(); // Очистка области под флажки.
    ui->Statewgt -> setResizeMode(QListView::Adjust);
    ui->Statewgt -> setMovement(QListView::Static); // Указываем, что элементы не могут быть перемещены пользователем.
    ui->Statewgt -> setSpacing(8);
    ui->Statewgt -> setFlow(QListView::LeftToRight); // Устанавливаем порядок расположения элементов слева на право.
    ui->Statewgt -> setWrapping(true); // Устанавливаем свойство, при  котором элементы переносятся, если выходят за область отображения.


        element = new QListWidgetItem("", ui->Statewgt); // Создаем новый экземпляр для элемента объекта QListWidget, указывая этот объект аргументом в качестве родителя.
        element -> setSizeHint(QSize(95,24));

        if (res_dio.at(5)=="00")
        {
            element->setBackground(green);
            element -> setText("Включен");
            state_old=res_dio.at(5);
        }
        else
        {
            element->setBackground(red);
            element -> setText("Выключен");
            state_old=res_dio.at(5);
        }

        QProcess beep;
        QString bep = "beep -f 2000 -n -f 1500 -n -f 1000";
        beep.start(bep);
        beep.waitForFinished(1000);
}


/************************* ПОДКЛЮЧЕНИЕ ДЛЯ ВКЛЮЧЕНИЯ **********************/
void MainWindow::connect_for_on()
{
   timer->stop(); // остановка таймера опроса
   on_sock->connectToHost(ip, port);
}

/********************************** Включение КРН *************************/
void MainWindow::turnOn()
{
    bool pas;
    QString str = QInputDialog::getText( 0, "Password", "Пароль:", QLineEdit::Password,"" ,&pas);

    if (!pas)
    {
        on_sock->close();
        timer->start(ask_timer);

        qDebug() << "Нажали кнопку \"включить\", и отмену на вводе пароля";
    }
    else
    {
        if (str=="569365")
        {
            control_timer->start(30000); // если за минуту не выполнится вся цепочка комманд, то диспетчер будет оповещен об этом.
            QByteArray set_dio; // команда установки состояний

            set_dio.resize(7); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100
            set_dio[0]=02;
            set_dio[1]=02;
            set_dio[2]=00;
            set_dio[3]=03;
            set_dio[4]=02; // номер канала 0, 1, 2, 3
            set_dio[5]=01; // установка канала в режим "output" , 00 - "input"
            set_dio[6]=00; // 01 - установка в "1", 00 - установка в "0"

            on_sock->write(set_dio); // отправка

            qDebug() << "отправка команды включения (нажать кнопку включения)";
        }
        else
        {
            QMessageBox::information(this, "Неверный пароль!", "Вы ввели неверный пароль!");
            on_sock->close();
            timer->start(ask_timer);
            qDebug() << "Был введен неверный пароль при попытке включения";
        }
    }
}


/************************* ПОДКЛЮЧЕНИЕ ДЛЯ ВЫКЛЮЧЕНИЯ **********************/
void MainWindow::connect_for_off()
{
   timer->stop(); // остановка таймера опроса
   off_sock->connectToHost(ip, 5001);
}

/********************************** Выключение КРН *************************/
void MainWindow::turnOff()
{
    bool pas;
    QString str = QInputDialog::getText( 0, "Password", "Пароль:", QLineEdit::Password,"" ,&pas);

    if (!pas)
    {
        off_sock->close();
        timer->start(ask_timer);
        qDebug() << "Нажали кнопку \"выключить\", и отмену на вводе пароля";
    }
    else
    {
        if (str=="569365")
        {
            control_timer->start(30000); // если за минуту не выполнится вся цепочка комманд, то диспетчер будет оповещен об этом.
            QByteArray set_dio; // команда установки состояний

            set_dio.resize(7); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100
            set_dio[0]=02;
            set_dio[1]=02;
            set_dio[2]=00;
            set_dio[3]=03;
            set_dio[4]=03; // номер канала 0, 1, 2, 3
            set_dio[5]=01; // установка канала в режим "output" , 00 - "input"
            set_dio[6]=00; // 01 - установка в "1", 00 - установка в "0"

            off_sock->write(set_dio); // отправка

            qDebug() << "отправка команды выключения (нажать кнопку выключения)";
        }
        else
        {
            QMessageBox::information(this, "Неверный пароль!", "Вы ввели неверный пароль!");
            off_sock->close();
            timer->start(ask_timer);
            qDebug() << "Был введен неверный пароль при попытке выключения";
        }
    }
}


/*************************** СБРОС УПРАВЛЯЕМЫХ КАНАЛОВ ******************/
void MainWindow::reset()
{
    QList<QString> res_management; // ответ от MOXA на попытку управления, пригодится при отладке
    QByteArray set_dio; // команда запроса состояний
    set_dio.resize(7); // записываем в массив команду. Описание команды см в инструкции к MOXA 4100

    res_management.clear(); // очищаем результат ответа

	
        QEventLoop loop;
        QTimer::singleShot(2000, &loop, SLOT(quit()));
        loop.exec();

    if (on_sock->isOpen())
    {
    int i=on_sock->bytesAvailable(); // количество байт, которое будет принято
    int j; // счетчик
    for (j=0; j<i; j++)
    {
        res_management << on_sock->read(1).toHex(); // Пишем в список побайтно ответ от MOXA
    }
    }
    else if (off_sock->isOpen()) {
        int i=off_sock->bytesAvailable(); // количество байт, которое будет принято
        int j; // счетчик
        for (j=0; j<i; j++)
        {
            res_management << off_sock->read(1).toHex(); // Пишем в список побайтно ответ от MOXA
        }

    }

    qDebug() << "ответ на попытку управления: " << res_management; // ответ от мохи, может помочь при отладке

    if (res_management.at(6)=="00")
    {
    if (on_sock->isOpen())
    {
        set_dio[0]=02;
        set_dio[1]=02;
        set_dio[2]=00;
        set_dio[3]=03;
        set_dio[4]=02; // номер канала 0, 1, 2, 3
        set_dio[5]=01; // установка канала в режим "output" , 00 - "input"
        set_dio[6]=01; // 01 - установка в "1", 00 - установка в "0"

        on_sock->write(set_dio); // отправка
    }
    else if (off_sock->isOpen()) {
        set_dio[0]=02;
        set_dio[1]=02;
        set_dio[2]=00;
        set_dio[3]=03;
        set_dio[4]=03; // номер канала 0, 1, 2, 3
        set_dio[5]=01; // установка канала в режим "output" , 00 - "input"
        set_dio[6]=01; // 01 - установка в "1", 00 - установка в "0"

        off_sock->write(set_dio); // отправка
    }
    }
    else
    {
        if (on_sock->isOpen())
        {
            on_sock->close();
            control_timer->stop();
            qDebug() << "сокет для включения закрыт";
            connect_to_moxa();
        }
        else if (off_sock->isOpen())
        {
            off_sock->close();
            control_timer->stop();
            qDebug() << "сокет для выключения закрыт";
            connect_to_moxa();
        }
    }
}

/***************************** ВЫВОД СООБЩЕНИЯ ОБ ОШИБКЕ УПРАВЛЕНИЯ ****************************/
void MainWindow::bad_connection()
{
    QMessageBox::information(this, "Что-то не так!", "С момента отправки команды прошло слишком много времени! \n Попробуйте еще раз, если опять увидите это сообщение, то необходимо выехать на объект");

    control_timer->stop();
    if (on_sock->isOpen())
    {
        on_sock->close();
        qDebug() << "вероятно что-то не так со связью, сокет для включения закрыт";
        connect_to_moxa();
    }
    else if (off_sock->isOpen())
    {
        off_sock->close();
        qDebug() << "вероятно что-то не так со связью, сокет для выключения закрыт";
        connect_to_moxa();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
