#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QFile>
#include <QDateTime>
#include <QDate>
#include <QDebug>
#include <QTextStream>

/*********************** Функция логирования *******************/

void myMessageOutput(QtMsgType type, const char *msg)
 {
    if (!QDir(qApp->applicationDirPath()+"/log").exists()) {
        QDir().mkdir(qApp->applicationDirPath()+"/log/");
    }
    QFile fMessFile(qApp->applicationDirPath() + "/log/" + QDate::currentDate().toString("dd.MM.yyyy") + ".log");
 if(!fMessFile.open(QIODevice::Append | QIODevice::Text)){
 return;
 }
 QString sCurrDateTime = "[" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") + "]";
 QTextStream tsTextStream(&fMessFile);
 switch(type){
 case QtDebugMsg:
 tsTextStream << QString("Debug%1: %2\n").arg(sCurrDateTime).arg(msg);
 break;
 case QtWarningMsg:
 tsTextStream << QString("Warning%1: %2\n").arg(sCurrDateTime).arg(msg);
 break;
 case QtCriticalMsg:
 tsTextStream << QString("Critical%1: %2\n").arg(sCurrDateTime).arg(msg);
 break;
 case QtFatalMsg:
 tsTextStream << QString("Fatal%1: %2\n").arg(sCurrDateTime).arg(msg);
 abort();
 }
 tsTextStream.flush();
 fMessFile.flush();
 fMessFile.close();
 }

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qInstallMsgHandler(myMessageOutput);
    qDebug() << " Program started";
    QCoreApplication::setOrganizationName("OOE");
    QCoreApplication::setApplicationName("krn_narishkino");
    MainWindow w;
    w.setWindowFlags(Qt::WindowStaysOnTopHint);
    w.show();

    return a.exec();
}
