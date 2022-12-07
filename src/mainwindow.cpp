#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "my_auth.h"
#include <customentry.h>

#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <iostream>

SecretKeyFlags runtimeFlag = SecretKeyFlags::base32_encoded_secretKey;

const QRegularExpression rx_base32("^(?:[A-Z2-7]{8})*(?:[A-Z2-7]{2}={6}|[A-Z2-7]{4}={4}|[A-Z2-7]{5}={3}|[A-Z2-7]{7}=)?$");
const QRegularExpression rx_uri("(otpauth)://(hotp|totp)/([^?#]*)\\?([^#]*)");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    const QValidator *v_base32 = new QRegularExpressionValidator(rx_base32, 0);
    const QValidator *v_uri = new QRegularExpressionValidator(rx_uri, 0);

    ui->secretKeyLineEdit->setValidator(v_base32);
    ui->uriLineEdit->setValidator(v_uri);

    std::pair<bool, std::string> dataFilePair = findDataFile();
    if(dataFilePair.first == true) {
        ui->mainStackedWidget->setCurrentIndex(1); // login page
        m_dataFilePath = dataFilePair.second;
    }
    else {
        ui->mainStackedWidget->setCurrentIndex(0); // new pass page
    }
}

MainWindow::~MainWindow()
{
    std::cout << "Window Destruction" << std::endl;
    MainWindow::saveToDatafile();
    delete ui;
}


void MainWindow::on_enterPassButton_clicked()
{
    QString ab = ui->enterPassField->text();
    if(statDataFile(m_dataFilePath) == true) {
        if(authenticatePassPhrase(m_dataFilePath, ab.toStdString())) {
            authenticatedIn = true;
            ui->enterPassField->setText("aaaaaa");
            ui->enterPassField->setText("");

            runtimeFlag = SecretKeyFlags::encrypted_secretKey;
            std::map<int, Uri> a = readAuthDB(m_dataFilePath, ab.toStdString());
            populate_mainStackedWidget(a);
            ui->mainStackedWidget->setCurrentIndex(2); // token page
        }
        else {
            ui->enterPassField->setText("aaaaaa");
            ui->enterPassField->setText("");
            QMessageBox msgBox;
            msgBox.setText("Authentication Failed! Please try again.");
            msgBox.exec();
        }
    }
    else {
        QMessageBox msgBox;
        msgBox.setText("Datafile not found.");
        msgBox.exec();
    }
}


void MainWindow::on_exportButton_clicked()
{
    std::map<int,Uri> uriMap;
    QString exportFilename = QFileDialog::getSaveFileName(this, tr("Save File"),
        QDir::homePath(), tr("Any File (*.*);;Text file (*.txt)"));

    for(int i = 0; i < ui->listWidget->count(); ++i)
    {
        QListWidgetItem* item = ui->listWidget->item(i);
        customEntry* entry = qobject_cast<customEntry*>(ui->listWidget->itemWidget(item));
        Q_ASSERT(entry);
        uriMap[i] = entry->getUri();
    }
    if(exportRawData(uriMap,exportFilename.toStdString())==true)
    {
        std::cout << "[Info] (Export) Data exported successfully" << std::endl;
    }
    else
    {
        std::cerr << "[Error] (Export) Data export failed" << std::endl;
    }
}


void MainWindow::on_enterNewPassButton_clicked()
{
    if(ui->newPasswordField->text() == ui->confirmPasswordField->text()) {
        convertToDatafile("", defaultConfigPath, ui->newPasswordField->text().toStdString());
        m_dataFilePath = defaultConfigPath;
        authenticatedIn = true;
        ui->mainStackedWidget->setCurrentIndex(2);
    }
    else {
        QMessageBox msgBox;
        msgBox.setText("The passwords do not match");
        msgBox.exec();
    }
}

void MainWindow::addMainStackedWidgetEntry(Uri uriEntry, SecretKeyFlags flag)
{
    auto item = new QListWidgetItem();
    auto widget = new customEntry(uriEntry, flag, this);

    item->setSizeHint(widget->sizeHint());

    ui->listWidget->addItem(item);
    ui->listWidget->setItemWidget(item, widget);
}

void MainWindow::populate_mainStackedWidget(std::map<int,Uri> uriMap)
{
    for(auto&& it:uriMap) {
        addMainStackedWidgetEntry(it.second, runtimeFlag);
    }
}

void MainWindow::saveToDatafile()
{
    if(dataChangedFlag == true && statDataFile(m_dataFilePath))
    {
        std::vector<std::string> uriVect;
        std::map<int,Uri> uriMap;
        for(int i = 0; i < ui->listWidget->count(); ++i)
        {
            QListWidgetItem* item = ui->listWidget->item(i);
            customEntry* entry = qobject_cast<customEntry*>(ui->listWidget->itemWidget(item));
            Q_ASSERT(entry);
            uriMap[i] = entry->getUri();
        }
        if(statDataFile(m_dataFilePath) && updateDatafile(uriMap, m_dataFilePath))
        {
            std::cout << "[Info] (Destructor) Data saved to file" << std::endl;
        }
        else
        {
            std::cerr << "[Error] (Destructor) Saving data to file failed" << std::endl;
        }
    }
}


void MainWindow::dataChanged()
{
    dataChangedFlag = true;
    std::cout << "Data Change Triggered" << std::endl;
}

void MainWindow::on_addEntryButton_clicked()
{
    ui->mainStackedWidget->setCurrentIndex(ui->mainStackedWidget->currentIndex()+1);
}


void MainWindow::on_removeEntryButton_clicked()
{
    QListWidgetItem * item = ui->listWidget->currentItem();
    delete ui->listWidget->takeItem(ui->listWidget->row(item));
    MainWindow::dataChanged();
}

void MainWindow::checkLineEdits()
{
    bool ok = true;
}


void MainWindow::manualAddUri()
{
    Uri a;
    a.protocol = "otpauth";
    //accountname
    a.labelAccountName = ui->accountLineEdit->text().toStdString();
    //issuer
    a.labelIssuer = ui->issuerLineEdit->text().toStdString();
    a.parameters.issuer = ui->issuerLineEdit->text().toStdString();
    //type
    a.otpType = ui->hotp_radioButton->isChecked() ? "hotp" : "totp";
    a.parameters.counter = a.otpType == "hotp" ? "0" : "";
    //Key
    a.parameters.secretKey = ui->secretKeyLineEdit->text().toStdString();
    //time
    if(a.otpType == "totp")
    {
        a.parameters.stepPeriod = std::to_string(ui->timerSpinBox->value());
    }
    //token digits
    a.parameters.codeDigits = std::to_string(ui->tokenLengthSpinBox->value());
    //hash
    if(ui->sha512_radioButton->isChecked())
    {
        a.parameters.hashAlgorithm = "SHA512";
    }
    else if(ui->sha256_radioButton->isChecked())
    {
        a.parameters.hashAlgorithm = "SHA256";
    }
    else
    {
        a.parameters.hashAlgorithm = "SHA1";
    }

    a = runtimeEncrypt(a);
    MainWindow::addMainStackedWidgetEntry(a, runtimeFlag);

    MainWindow::clearAddForm();
    ui->mainStackedWidget->setCurrentIndex(ui->mainStackedWidget->currentIndex()-1);
    MainWindow::dataChanged();
}

void MainWindow::stringAddUri()
{
    QString uriString = ui->uriLineEdit->text();
    if(!uriString.trimmed().isEmpty())
    {
        Uri a = parseUriString(uriString.toStdString());
        a = runtimeEncrypt(a);
        MainWindow::addMainStackedWidgetEntry(a, runtimeFlag);

        MainWindow::clearAddForm();
        ui->mainStackedWidget->setCurrentIndex(ui->mainStackedWidget->currentIndex()-1);
        MainWindow::dataChanged();
    }
}

void MainWindow::on_comfirmAddEntryButton_clicked()
{
    if(ui->uriRadioButton->isChecked())
    {
        MainWindow::stringAddUri();
    }
    else if(ui->manualRadioButton->isChecked())
    {
        MainWindow::manualAddUri();
    }
}


void MainWindow::on_cancelAddEntryButton_clicked()
{
    MainWindow::clearAddForm();
    ui->mainStackedWidget->setCurrentIndex(ui->mainStackedWidget->currentIndex()-1);
}

void MainWindow::clearAddForm()
{
    ui->manualRadioButton->setChecked(true);
    ui->accountLineEdit->setText("");
    ui->issuerLineEdit->setText("");
    ui->totp_radioButton->setChecked(true);
    ui->secretKeyLineEdit->setText("");
    ui->timerSpinBox->setValue(30);
    ui->tokenLengthSpinBox->setValue(6);
    ui->sha1_radioButton->setChecked(true);
    ui->uriLineEdit->setText("");
}
