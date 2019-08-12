#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include "QMessageBox"
#include "QDebug"
#include "QTableWidget"
#include "QComboBox"
#include "send/FileSendControl.h"
#include "util/zip.h"
#include <functional>
#include <boost/uuid/random_generator.hpp>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    console = FileSendControl::GetInstances();
    console->RegistNoticeFrontCallBack(
                std::bind(
                    NoticeFrontCallBack,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3,
                    this
                    )
                );

    console->Run();
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_sendFile_clicked()
{
    QString openFile="";
    openFile=QFileDialog::getOpenFileName(this,"打开","./","");
    if (openFile == "") return ;
    console->SendFile(openFile.toStdString());
}

void MainWindow::on_sendDir_clicked()
{
    QString openDir="";
    openDir = QFileDialog::getExistingDirectory(this, "open");
    if (openDir == "") return ;
    console->SendFile(openDir.toStdString());
}

void MainWindow::NoticeFrontCallBack(const boost::uuids::uuid file_uuid,
                                     const FileSendControl::Type type,
                                     const std::vector<std::string> msg,
                                     MainWindow *window) {
    auto table = window->ui->tableWidget;
    QTableWidgetItem *item_file = nullptr;
    QTableWidgetItem *item_stat = nullptr;
    QString file_name;
    int rowIndex = -1;
    int rowCnt = 0;
    switch (type) {
    case FileSendControl::kNewFile:
        rowIndex = table->rowCount();
        table->setRowCount(rowIndex+1);
        file_name = QString::fromStdString(msg.front());
        item_file = new QTableWidgetItem(file_name);
        item_stat = new QTableWidgetItem("recving");
        {std::lock_guard<std::mutex> lock(window->mutex_);
            window->index_[file_uuid] = rowIndex;
        }
        table->setItem(rowIndex, 0, item_file);
        table->setItem(rowIndex, 1, item_stat);
        break;
    case FileSendControl::kRecvend:
        {std::lock_guard<std::mutex> lock(window->mutex_);
            auto it = window->index_.find(file_uuid);
            if (it != window->index_.end()) {
                rowIndex = window->index_[file_uuid];
                window->index_.erase(it);
            } else {
                break;
            }
        }  //end lock
        item_file = table->item(rowIndex, 0);
        if (item_file) {
            item_stat = table->item(rowIndex, 1);
        }
        if (item_stat) {
            item_stat->setText("recvend");
        } else {
            qDebug() << "not found " << file_name;
        }
        break;
    case FileSendControl::kSendend:
        {std::lock_guard<std::mutex> lock(window->mutex_);
            auto it = window->index_.find(file_uuid);
            if (it != window->index_.end()) {
                rowIndex = window->index_[file_uuid];
                window->index_.erase(it);
            } else {
                break;
            }
        }  //end lock
        item_file = table->item(rowIndex, 0);
        if (item_file) {
            item_stat = table->item(rowIndex, 1);
        }
        if (item_stat) {
            item_stat->setText("sendend");
        }
        break;
    case FileSendControl::kNewSendFile:
        rowIndex = table->rowCount();
        table->setRowCount(rowIndex+1);
        file_name = QString::fromStdString(msg.front());
        item_file = new QTableWidgetItem(file_name);
        item_stat = new QTableWidgetItem("sending");
        {std::lock_guard<std::mutex> lock(window->mutex_);
            window->index_[file_uuid] = rowIndex;
        }
        table->setItem(rowIndex, 0, item_file);
        table->setItem(rowIndex, 1, item_stat);
    default :
        qDebug() << "not found type";
    }
}
