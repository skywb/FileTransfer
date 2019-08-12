#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QTableWidget"
#include "send/FileSendControl.h"
#include <boost/uuid/uuid.hpp>

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
    static void NoticeFrontCallBack(const boost::uuids::uuid file_uuid,
                                    const FileSendControl::Type type,
                                    const std::vector<std::string>
                                    , MainWindow *window);
    void on_sendFile_clicked();

    void on_sendDir_clicked();

private:
    Ui::MainWindow *ui;
    FileSendControl *console;
    std::mutex mutex_;
    std::map<boost::uuids::uuid, int> index_;
};

#endif // MAINWINDOW_H
