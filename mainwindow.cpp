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
                    ui->tableWidget
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
    auto table = ui->tableWidget;
    openFile=QFileDialog::getOpenFileName(this,"打开","./","");
    if (openFile == "") return ;
    int rowIndex = table->rowCount();
    table->setRowCount(rowIndex + 1);//总行数增加1
    //ui->tableWidget->setRowHeight(rowIndex, 24);//设置行的高度
    QString file_name = openFile;
    file_name = file_name.mid(file_name.lastIndexOf('/')+1, file_name.size());
    QTableWidgetItem *item = new QTableWidgetItem(file_name);
    table->setItem(rowIndex, 0, item);
    item = new QTableWidgetItem(QString("sending"));
    table->setItem(rowIndex, 1, item);
    //item->setText("item");
    console->SendFile(openFile.toStdString());
}

void MainWindow::on_sendDir_clicked()
{
    QString openDir="";
    auto table = ui->tableWidget;
    //openDir = QFileDialog::getOpenFileName(this,"open","./","");
    openDir = QFileDialog::getExistingDirectory(this, "open");
    if (openDir == "") return ;
    zip(openDir.toStdString());
    int rowIndex = table->rowCount();
    table->setRowCount(rowIndex + 1);//总行数增加1
    QString file_name = openDir;
    file_name = file_name.mid(file_name.lastIndexOf('/')+1, file_name.size());
    QTableWidgetItem *item = new QTableWidgetItem(file_name);
    table->setItem(rowIndex, 0, item);
    item = new QTableWidgetItem(QString("sending"));
    table->setItem(rowIndex, 1, item);
    console->SendFile(openDir.toStdString());
}

void MainWindow::NoticeFrontCallBack(std::string fileName, FileSendControl::Type type, QTableWidget *table) {
    QString file_name = QString::fromStdString(fileName);
    QTableWidgetItem *item_file = nullptr;
    QTableWidgetItem *item_stat = nullptr;

    int rowIndex = -1;
    int rowCnt = 0;
    switch (type) {
    case FileSendControl::kNewFile:
        rowIndex = table->rowCount();
        table->setRowCount(rowIndex+1);
        item_file = new QTableWidgetItem(file_name);
        item_stat = new QTableWidgetItem("recving");
        table->setItem(rowIndex, 0, item_file);
        table->setItem(rowIndex, 1, item_stat);
        break;
    case FileSendControl::kRecvend:
        rowCnt = table->rowCount();
        for (int i=0; i<rowCnt; ++i) {
            item_file = table->item(i, 0);
            if (item_file->text() == file_name) {
                item_stat = table->item(i, 1);
                break;
            }
        }
        if (item_stat) {
            item_file->setText(file_name);
            item_stat->setText("recvend");
        } else {
            qDebug() << "not found " << file_name;
        }
        break;
    case FileSendControl::kSendend:
        rowCnt = table->rowCount();
        for (int i=0; i<rowCnt; ++i) {
            item_file = table->item(i, 0);
            if (item_file->text() == file_name) {
                rowIndex = i;
                item_stat = table->item(i, 1);
                break;
            }
        }
        if (item_stat) {
            item_file->setText(file_name);
            item_stat->setText("sendend");
        }
        break;
    }
}
