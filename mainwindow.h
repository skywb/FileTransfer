#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QTableWidget"
#include "send/FileSendControl.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    //void sendFile(FileSendControl* ctl, std::string filepath);
    //bool recvFile(std::string savePath);
    static void NoticeFrontCallBack(std::string fileName, FileSendControl::Type type, QTableWidget *table);
    void on_sendFile_clicked();

    void on_sendDir_clicked();

private:
    Ui::MainWindow *ui;
    FileSendControl *console;
};

#endif // MAINWINDOW_H
