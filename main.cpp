#define DEBUG true
#include "mainwindow.h"
#include <QApplication>
#include <glog/logging.h>

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = "./";
    LOG(INFO) << "start...";
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    google::ShutdownGoogleLogging();
    return a.exec();
}
