#ifndef TIMETRACKER_H
#define TIMETRACKER_H

#include <QWidget>
#include <QtSql>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QLabel>
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QTime>
#include <QScreen>
#include <QPixmap>
#include <QCryptographicHash>
#include <QBuffer>
#include <QByteArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QMutex>
#include <QGridLayout>
#include <QScrollArea>
#include <QToolBar>
#include <QAction>

#include "screenshotgriditem.h"

#define THREAD_NUM 1

QT_BEGIN_NAMESPACE
namespace Ui { class TimeTracker; }
QT_END_NAMESPACE

class TimeTracker : public QWidget
{
    Q_OBJECT

public:
    TimeTracker(QWidget *parent = nullptr);
    ~TimeTracker();

private slots:
    void startEndButtonClicked();
    void takeScreenshot();
    QByteArray pixmapToByteArray(const QPixmap &pixmap);
    //QString calculateHash(const QByteArray &byteArray);
    void uploadToDatabase(const QByteArray &screenByteArray, const qint64 hashSum, const double similarity);
    qint64 calculateHashSum(const QByteArray &byteArray);
    //double calculateSimilaritySSIM(const QByteArray &image1, const QByteArray &image2);
    double calculateSimilarity(QByteArray &image1);
    void updateGridView();
    void clearGridLayout(QGridLayout *gridLayout);
    void setLastScreenFromDB();
signals:
    void newItemAdded();
private:
    Ui::TimeTracker *ui;
    QTimer *timer;
    QByteArray previousScreen;
    QMutex similarityMutex;
    QWidget screenGridItem;
    QSqlDatabase timeTrackerDB;
    QGridLayout *screensGridLayout;
    QAction *startEndAction;
};
#endif // TIMETRACKER_H
