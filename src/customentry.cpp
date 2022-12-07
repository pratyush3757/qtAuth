#include "customentry.h"
#include "ui_customentry.h"

#include "ProgressCircle.h"

#include "token.h"

#include <QPainter>
#include <QDateTime>

#include <iostream>

customEntry::customEntry(Uri uri,
                         SecretKeyFlags runtimeFlag,
                         QWidget *parent) :
    QWidget(parent),
    ui(new Ui::customEntry),
    m_uri(uri),
    m_runtimeFlag(runtimeFlag),
    m_circleMaxVal(360)
{
    ui->setupUi(this);
    QWidget *horizontalLineWidget = new QWidget;
    horizontalLineWidget->setFixedHeight(1);
    horizontalLineWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    horizontalLineWidget->setStyleSheet(QString("background-color: #c0c0c0;"));
    ui->gridLayout->addWidget(horizontalLineWidget, 2, 0, 1, 3);

    m_otpType = m_uri.otpType;
    m_issuer = m_uri.labelIssuer;
    m_username = m_uri.labelAccountName;
    m_codeDigits = (m_uri.parameters.codeDigits=="") ?
                    6 : stoi(m_uri.parameters.codeDigits);
    m_stepPeriod = (m_uri.parameters.stepPeriod=="") ?
                    30 : stoi(m_uri.parameters.stepPeriod);
    m_hashAlgorithm = (m_uri.parameters.hashAlgorithm=="") ?
                       "SHA1" : m_uri.parameters.hashAlgorithm;
    long long int curr_time = QDateTime::currentSecsSinceEpoch();

    if(m_otpType == "hotp")
    {
        m_hotpCounter = (m_uri.parameters.counter=="") ?
                    0 : stoi(m_uri.parameters.counter);
        std::string hotp = computeHotp(m_uri.parameters.secretKey,
                                       m_hotpCounter,
                                       m_codeDigits,
                                       m_hashAlgorithm,
                                       m_runtimeFlag);

        m_nextCounter = new QPushButton();
        m_nextCounter->setText(u8"\u2192");
        m_nextCounter->setToolTip("Next Token");
        m_nextCounter->setFixedSize(45, 45);
        ui->gridLayout->addWidget(m_nextCounter, 0, 2, 2, 1);
        connect(m_nextCounter, SIGNAL(clicked()), this, SLOT(hotpButtonClicked()));

        connect(this, SIGNAL(dataChangedSig()), parent, SLOT(dataChanged()));

        ui->tokenLabel->setText(QString::fromStdString(hotp));
    }
    else if(m_otpType == "totp")
    {
        std::string totp = computeTotp(m_uri.parameters.secretKey,
                                       curr_time,
                                       m_codeDigits,
                                       m_hashAlgorithm,
                                       m_stepPeriod,
                                       m_runtimeFlag);
        m_lifetime = computeTotpLifetime(curr_time, m_stepPeriod);

        m_progressCirclePtr = new ProgressCircle();
        m_progressCirclePtr->setFixedSize(45, 45);
        m_progressCirclePtr->setMaximum(m_circleMaxVal);
        ui->gridLayout->addWidget(m_progressCirclePtr, 0, 2, 2, 1);

        m_timerAnimation = new QPropertyAnimation();
        m_updateTimer = new QTimer(this);
        customEntry::startTimerAnimation();
        ui->tokenLabel->setText(QString::fromStdString(totp));
    }

    ui->issuerLabel->setText(QString::fromStdString(m_issuer));
    ui->usernameLabel->setText(QString::fromStdString(m_username));
}

customEntry::~customEntry()
{
    delete ui;
}

void customEntry::setToken(std::string token)
{
    ui->tokenLabel->setText(QString::fromStdString(token));
}

void customEntry::updateTotp() {
    std::string totp = computeTotp(m_uri.parameters.secretKey,
                                   QDateTime::currentSecsSinceEpoch(),
                                   m_codeDigits,
                                   m_hashAlgorithm,
                                   m_stepPeriod,
                                   m_runtimeFlag);
    customEntry::setToken(totp);
}

std::string customEntry::getUriString() {
    return deriveUriString(m_uri);
}

Uri customEntry::getUri(){
    return m_uri;
}

void customEntry::hotpButtonClicked() {
    m_hotpCounter += 1;
    m_uri.parameters.counter = std::to_string(m_hotpCounter);
    std::string hotp = computeHotp(m_uri.parameters.secretKey,
                                   m_hotpCounter,
                                   m_codeDigits,
                                   m_hashAlgorithm,
                                   m_runtimeFlag);
    customEntry::setToken(hotp);
    emit dataChangedSig();
}

void customEntry::startTimerAnimation()
{
        double startVal = ((double)m_lifetime/m_stepPeriod)*m_circleMaxVal;
        m_progressCirclePtr->setValue(startVal);
        m_timerAnimation->setTargetObject(m_progressCirclePtr);
        m_timerAnimation->setPropertyName("value");
        m_timerAnimation->setEasingCurve(QEasingCurve::Linear);
        m_timerAnimation->setStartValue(startVal);
        m_timerAnimation->setEndValue(0);
        m_timerAnimation->setDuration(m_lifetime*1000);
        m_timerAnimation->setLoopCount(-1);
        m_timerAnimation->start();
        QTimer::singleShot(m_lifetime*1000, this, &customEntry::changeAnimationDuration);
}

void customEntry::changeAnimationDuration()
{
    m_timerAnimation->stop();
    m_timerAnimation->setStartValue(m_circleMaxVal);
    m_timerAnimation->setDuration(m_stepPeriod*1000);
    m_timerAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(updateTotp()));
    m_updateTimer->start(m_stepPeriod*1000);
    customEntry::updateTotp();
}
