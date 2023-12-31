/**
 * Copyright (C) 2012 Jean Guegant (Jiwan)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QFileDialog>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_activated()
{
    QString path = QFileDialog::getOpenFileName(this,"Open your log file",NULL,tr("LOL logs (*.pkt)"));

    if (!path.isNull())
    {
        m_file.load(path);
    }

    ui->packetTreeView->reset();
    QVector<LogFile::Packet> packets = m_file.getPackets();
    QStandardItemModel *model = new QStandardItemModel(this);

    foreach (LogFile::Packet pack,packets)
    {
        QList<QStandardItem*> row;
        QStandardItem* typeItem;
        if (pack.type == LogFile::PT_CLIENT_TO_SERVER)
        {
            typeItem = new QStandardItem(QString("Client -> Server"));
        }
        else
        {
            typeItem = new QStandardItem(QString("Server -> Client"));
        }

        QStandardItem* sizeItem = new QStandardItem(QString("0x%1").arg(pack.length,0,16));

        QStandardItem* timeItem = new QStandardItem(tr("%0:%1:%2:%3").arg(QString::number(pack.time.wHour),QString::number(pack.time.wMinute),QString::number(pack.time.wSecond),QString::number(pack.time.wMilliseconds)));

        row << typeItem << sizeItem << timeItem;
        model->appendRow(row);
        row.clear();
    }
    model->setHeaderData(0,Qt::Horizontal,"Source -> Destination",Qt::DisplayRole);
    model->setHeaderData(1,Qt::Horizontal,"Length",Qt::DisplayRole);
    model->setHeaderData(2,Qt::Horizontal,"Time",Qt::DisplayRole);
    ui->packetTreeView->setModel(model);
    connect(ui->packetTreeView->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &,const QItemSelection &)),this,SLOT(on_packetsTreeView_selectionChanged(const QItemSelection &,const QItemSelection &))); //on changed index
}

void MainWindow::on_packetsTreeView_selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected )
{
    //what a fugly function
    QModelIndex index = ui->packetTreeView->selectionModel()->currentIndex();
    ui->packetDataTextBrowser->clear();
    LogFile::Packet p = m_file.getPackets().at(index.row());

    int row = p.data.size() / 16;
    int rest = p.data.size() % 16;

    QString data("");

    for (int i = 0; i < row; ++i)
    {
        data += QString("%1     ").arg(i * 16, 4, 16,QChar('0'));
        for (int j = 0; j < 16 ; ++j)
        {
            data += QString("%1 ").arg(static_cast<unsigned char>(p.data.at(i * 16 + j)),2,16,QChar('0'));
        }

        data += "       ";

        for (int j = 0; j < 16 ; ++j)
        {
            unsigned char c = p.data.at(i*16 + j);

            if (isprint(c))
            {
                data += c;
            }
            else
            {
                data +='.';
            }
        }
        data += "\n";
    }

    if (rest != 0)
    {
        data += QString("%1     ").arg(row * 16, 4, 16,QChar('0'));
        for (int i = 0; i < rest ; ++i)
        {
            data += QString("%1 ").arg(static_cast<unsigned char>(p.data.at(row*16 + i)),2,16,QChar('0'));
            if (i % 2 == 1)
            {
                data += " ";
            }
        }

        data += "       ";

        for (int i = 0; i < rest ; ++i)
        {
            unsigned char c = p.data.at(row*16 + i);

            if (isprint(c))
            {
                data += c;
            }
            else
            {
                data +='.';
            }
        }
    }

    ui->packetDataTextBrowser->setText(data);
}

void MainWindow::on_actionAbout_us_activated()
{
    QMessageBox::information(this,"About us",tr("This tools was created to parse log data generated by LOLProxy for LOLEMU project.\nLOLProxy : https://github.com/Jiwan\nLOLEMU : http://lolemu.tk/forum\nCredits to Jiwan. Thanks to Intline9 for his help."));
}
