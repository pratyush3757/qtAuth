#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "datatypes_uri.h"
#include "datatypes_flags.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void dataChanged();

private slots:
    void on_enterPassButton_clicked();

    void on_exportButton_clicked();

    void on_enterNewPassButton_clicked();

    void addMainStackedWidgetEntry(Uri uriEntry, SecretKeyFlags flag);

    void populate_mainStackedWidget(std::map<int,Uri> uriMap);

    void saveToDatafile();

    void on_addEntryButton_clicked();

    void on_removeEntryButton_clicked();

    void checkLineEdits();

    void manualAddUri();

    void stringAddUri();

    void on_comfirmAddEntryButton_clicked();

    void on_cancelAddEntryButton_clicked();

    void clearAddForm();

private:
    Ui::MainWindow *ui;

    bool authenticatedIn = false;
    bool dataChangedFlag = false;

    std::string m_dataFilePath;
    std::string defaultConfigPath = "test.dat";
};
#endif // MAINWINDOW_H
