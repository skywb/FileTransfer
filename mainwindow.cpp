#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include "QMessageBox"
#include "QDebug"
#include "QTableWidget"
#include "QComboBox"
#include "send/FileSendControl.h"
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
    int rowIndex = table->rowCount();
    table->setRowCount(rowIndex + 1);//总行数增加1
    //ui->tableWidget->setRowHeight(rowIndex, 24);//设置行的高度
    QTableWidgetItem *item = new QTableWidgetItem(openFile);
    //item->setText("item");
    console->SendFile(openFile.toStdString());
    //sendFile(openFile.toStdString());
    table->setItem(rowIndex, 0, item);
}

void MainWindow::on_sendDir_clicked()
{
    QString openDir="";
    auto table = ui->tableWidget;
    //openDir = QFileDialog::getOpenFileName(this,"open","./","");
    openDir = QFileDialog::getExistingDirectory(this, "open");
    int rowIndex = table->rowCount();
    table->setRowCount(rowIndex + 1);//总行数增加1
    //ui->tableWidget->setRowHeight(rowIndex, 24);//设置行的高度
    QTableWidgetItem *item = new QTableWidgetItem(openDir);
    console->SendFile(openDir.toStdString());
    //item->setText("item");
    table->setItem(rowIndex, 0, item);
}

void MainWindow::NoticeFrontCallBack(std::string fileName, FileSendControl::Type type, QTableWidget *table) {
    QString file_name;
    file_name.fromStdString(fileName);
    QTableWidgetItem *item_file = nullptr;
    QTableWidgetItem *item_stat = nullptr;

    int rowIndex = -1;
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
        int cnt = table->rowCount();
        for (int i=0; i<cnt; ++i) {
            item_file = table->item(i, 0);
            if (item_file->text() == file_name) {
                rowIndex = i;
                item_stat = table->item(i, 1);
                break;
            }
        }
        if (item_file) {
            item_file->setText(file_name);
            item_file->setText("recvend");
        }
        break;
    }
}
