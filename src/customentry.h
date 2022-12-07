#ifndef CUSTOMENTRY_H
#define CUSTOMENTRY_H

#include "ProgressCircle.h"

#include "datatypes_uri.h"
#include "datatypes_flags.h"

#include <QPropertyAnimation>
#include <QWidget>
#include <QTimer>
#include <QPushButton>

namespace Ui {
class customEntry;
}

class customEntry : public QWidget
{
    Q_OBJECT

public:
    explicit customEntry(Uri uri, SecretKeyFlags runtimeFlag, QWidget *parent = nullptr);
    ~customEntry();

signals:
    void dataChangedSig();

public slots:
    void setToken(std::string token);
    void updateTotp();
    std::string getUriString();
    Uri getUri();
    void hotpButtonClicked();

private:
    Ui::customEntry *ui;

    Uri m_uri;
    SecretKeyFlags m_runtimeFlag;

    std::string m_otpType;
    std::string m_issuer;
    std::string m_username;
    std::string m_hashAlgorithm;
    int m_codeDigits;
    long long int m_hotpCounter;
    int m_stepPeriod;
    int m_lifetime;
    int m_circleMaxVal;

    ProgressCircle * m_progressCirclePtr;
    QPropertyAnimation * m_timerAnimation;
    QTimer * m_updateTimer;
    QPushButton * m_nextCounter;

    void startTimerAnimation();
    void changeAnimationDuration();
};

#endif // CUSTOMENTRY_H
