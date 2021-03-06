/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "form.h"
#include "configdialog.h"

#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QFile>
#include <QFileDialog>

#include <QtCore/QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    runStatus(true),  // 上电运行
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    form = new Form;
    setCentralWidget(form);
    setWindowTitle(QWidget::tr("步进电机快速调试程序"));

    serial = new QSerialPort(this);

    settings = new SettingsDialog;
    config = new ConfigDialog;

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);

    ui->actionConfigure->setEnabled(true);
    ui->actionQuit->setEnabled(true);

    status = new QLabel;
    ui->statusBar->addWidget(status);

    initActionsConnections();

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,
            SLOT(handleError(QSerialPort::SerialPortError)));

    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));

    connect(form, SIGNAL(sendData(QByteArray)), this, SLOT(writeData(QByteArray)));
    connect(form, SIGNAL(sendStop(QByteArray)), this, SLOT(writeStop(QByteArray)));

    connect(config, SIGNAL(sendConfig(QByteArray)), this, SLOT(writeConfig(QByteArray)));

    connect(config, SIGNAL(changeConfigs()), this, SLOT(updateConfigs()));
}

MainWindow::~MainWindow()
{
    delete settings;
    delete config;
    delete form;
    delete ui;
}

void MainWindow::updateConfigs()
{
    //qDebug() << "mainwindow test slots";
    ConfigDialog::Configs c = config->configs();
    form->updateConfigs(c.elecLevel, c.circleLen, c.deviceId);
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {

        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        /*
        showStatusMessage(tr("连接 %1 : 波特率 %2, %3位, 校验%4, 停止位%5, 控制流%6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
                          */
        QString status = tr("串口连接成功\n端口%1, 波特率%2, 数据%3位, 校验方式 %4, 停止位%5位, 控制流%6")
                .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl);
        QMessageBox::information(this,tr("串口连接"),status);
    } else {
        QMessageBox::critical(this, tr("串口连接错误"), serial->errorString());

        showStatusMessage(tr("连接错误"));
    }
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
    {
        serial->close();
    }

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    //showStatusMessage(tr("断开连接"));
    QMessageBox::information(this, tr("串口连接"), tr("串口已断开连接"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Simple Terminal"),
                       tr("The <b>Simple Terminal</b> example demonstrates how to "
                          "use the Qt Serial Port module in modern GUI applications "
                          "using Qt, with a menu bar, toolbars, and a status bar."));
}

void MainWindow::writeData(const QByteArray &data)
{
    //串口发送数据 - 缓存写数据
    //showStatusMessage(tr("发送指令数据：") + data.toHex());
    //qDebug() << data.toHex();

    if(!serial->isOpen())
    {
        QMessageBox::warning(this,tr("警告"),tr("请打开串口！"));
        return;
    }
    //未接收到停止状态反馈不能下载程序
    if(runStatus) {
        QMessageBox::warning(this, tr("警告"), tr("请停止运行"));
        return;
    }

    serial->write(data);
}

//停止专用
void MainWindow::writeStop(const QByteArray &data)
{

    if(!serial->isOpen())
    {
        QMessageBox::warning(this,tr("警告"),tr("请打开串口！"));
        return;
    }

    serial->write(data);
}

void MainWindow::writeConfig(const QByteArray &data)
{
    //showStatusMessage(tr("发送指令数据：") + data.toHex());

    if(!serial->isOpen())
    {
        QMessageBox::warning(this,tr("警告"),tr("请打开串口！"));
        return;
    }

    serial->write(data);

    config->tip();
}

void MainWindow::readData()
{
    //缓存读数据 - 串口接收数据
    QByteArray recv = serial->readAll();
    int size = recv.size();
    //qDebug() << QString::fromLatin1(data.toHex().data());


    if(size > 2) {

        int ret = (quint8) recv.at(2);
        //qDebug() << QString::number(ret);
        switch (ret) {
        case BATCHCONFCMD:
            QMessageBox::information(this, tr("下载提示"), tr("下载配置成功！"));
            break;
        case CMDBATCHHEAD:
            runStatus = true; //设备运行状态
            QMessageBox::information(this, tr("下载提示"), tr("下载程序成功！"));
            break;
        case EMSTOP_CMD:
            runStatus = false; //设备停止状态
            QMessageBox::information(this, tr("设备状态"), tr("设备已停止"));
        default:
            //QMessageBox::information(this, tr("下载提示"), tr("下载数据成功！"));
            break;
        }
    }

}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(openSerialPort()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(closeSerialPort()));
    connect(ui->actionConfigure, SIGNAL(triggered()), settings, SLOT(show()));
    connect(ui->actionSystem, SIGNAL(triggered()), config, SLOT(show()));
    connect(ui->actionAbout, SIGNAL(triggered()), form, SLOT(about()));

    connect(ui->actionOpenProg, SIGNAL(triggered(bool)), this, SLOT(openProgFile()));
    connect(ui->actionSaveProg, SIGNAL(triggered(bool)), this, SLOT(saveProgFile()));

    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(closeAll()));
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
    //statusBar()->showMessage(message, 1000);
}

void MainWindow::closeAll()
{
    close();
    config->close();
    settings->close();
}

void MainWindow::closeEvent(QCloseEvent */*e*/)
{
    closeAll();
}

void MainWindow::openProgFile()
{
    //打开默认文件目录等待用户选中载入文件，限制打开文件格式
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开程序文件"),
                               "",
                               tr("程序 (*.prog)"));
    //qDebug() << "打开文件名： " + fileName;
    //读取文件流内容，调用程序form载入程序指令方法
    if(form->loadProgFile(fileName))
        qDebug() << "load prog file successful.";
    else
        qDebug() << "load failed.";
}

void MainWindow::saveProgFile()
{
    //打开文件保存对话框，提示用户输入文件名以待保存
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存程序文件"),
                               "程序.prog",
                               tr("程序 (*.prog)"));

    bool ret = form->saveProgFile(fileName);

    //保存为磁盘文件
    if(ret) {
        qDebug() << "save prog file successful.";
    } else {
        qDebug() << "save prog file failed.";
    }
}
