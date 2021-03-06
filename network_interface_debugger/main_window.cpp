#include "main_window.h"
#include "ui_main_window.h"
#include <QDebug>
#include "udp_socket.h"
#include <QMessageBox>
#include <iostream>
#include <QThread>
#include "tcp_socket.h"

using namespace std;

const QString kWarnDialogTitle = "Warn";
const QString kErrorDialogTitle = "Error";
const QString kInfoDialogTitle = "Info";

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  sampler_.reset(new IncomingPacketSampler);
  SetUISocketOptionUDP();
  SetSocketFromUI();
  text_sender_.reset(new TextSender);
  text_sender_->SetUITextEdit(ui->SenderPlainTextEdit);
  text_sender_->SetUILineEdit(ui->SenderLineEdit);
  text_sender_->SetMessageWrapFlag(ui->WrapMessageCheckBox->isChecked());
  text_receiver_.reset(new TextReceiver);
  text_receiver_->SetUITextEdit(ui->ReceiverPlainTextEdit);
  timer_id_ = startTimer(100);
}

MainWindow::~MainWindow()
{
  if (socket_) {
    socket_->Shut();
  }
  delete ui;
}

void MainWindow::timerEvent(QTimerEvent *event) {
  if (event->timerId() == timer_id_) {
    auto reports = socket_->AsyncReceive();
    if (reports.empty()) {
      return;
    }
    for (auto& report : reports) {
      text_receiver_->DisplayMessageReport(report);
    }
  }
}

void MainWindow::SetUISocketOptionTCP() {
  ui->TCPCheckBox->setChecked(true);
  ui->UDPCheckBox->setChecked(false);
}

void MainWindow::SetUISocketOptionUDP() {
  ui->TCPCheckBox->setChecked(false);
  ui->UDPCheckBox->setChecked(true);
}

QString MainWindow::GetIPStringFromUI() {
  return
      ui->IPLineEdit1->text() + "." +
      ui->IPLineEdit2->text() + "." +
      ui->IPLineEdit3->text() + "." +
      ui->IPLineEdit4->text();
}

quint16 MainWindow::GetPortFromUI() {
  return
    ui->PortLineEdit->text().toInt();
}

void MainWindow::SetSocketFromUI() {
  if (socket_) {
    socket_->Shut();
  }
  if (ui->TCPCheckBox->isChecked()) {
    socket_.reset(new TCPSocket);
    socket_->SetIP(GetIPStringFromUI());
    socket_->SetPort(GetPortFromUI());
    socket_->Initialize();
    sampler_->SetNetworkServer(socket_);
  } else if (ui->UDPCheckBox->isChecked()) {
    if (socket_) {
      socket_->Shut();
    }
    socket_.reset(new UDPSocket);
    socket_->SetIP(GetIPStringFromUI());
    socket_->SetPort(GetPortFromUI());
    socket_->Initialize();
    sampler_->SetNetworkServer(socket_);
  } else {
    QMessageBox::critical(
        this, kErrorDialogTitle,
        "Neither UDP nor TCP is selected, setting UDP socket as default.",
        QMessageBox::Ok);
    SetUISocketOptionUDP();
    if (socket_) {
      socket_->Shut();
    }
    socket_.reset(new UDPSocket);
    socket_->Initialize();
    sampler_->SetNetworkServer(socket_);
  }
}

void MainWindow::on_TCPCheckBox_clicked()
{
  ui->UDPCheckBox->setChecked(!ui->TCPCheckBox->isChecked());
  ui->PortLineEdit->setText("50660");
  SetSocketFromUI();
}

void MainWindow::on_UDPCheckBox_clicked()
{
  ui->TCPCheckBox->setChecked(!ui->UDPCheckBox->isChecked());
  ui->PortLineEdit->setText("2368");
  SetSocketFromUI();
}

void MainWindow::on_SendPushButton_clicked()
{
  if (!socket_) {
    cerr << "No socket has been established." << endl;
    QApplication::exit(-1);
  }
  auto command = text_sender_->GetMessageString();
  if (sampler_->InspectCommand(command)) {
    QMessageBox::information(
        this, kInfoDialogTitle,
        "The command can cause packet overflow, "
        "receiving frequency has been decreased to " +
        QString::number(sampler_->Frequency()) + " Hz.");
  }
  text_sender_->DisplayMessageReport(
      socket_->AsyncSend(command));
}

void MainWindow::on_WrapMessageCheckBox_clicked(bool checked)
{
  text_sender_->SetMessageWrapFlag(checked);
  ui->WrapMessagePushButton->setDisabled(checked);
}

void MainWindow::on_WrapMessagePushButton_clicked()
{
  text_sender_->WrapMessageNow();
}

void MainWindow::on_IPPortApplyPushButton_clicked()
{
  SetSocketFromUI();
}
