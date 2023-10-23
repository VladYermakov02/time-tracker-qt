#ifndef SCREENSHOTGRIDITEM_H
#define SCREENSHOTGRIDITEM_H

#include <QWidget>

namespace Ui {
class screenshotGridItem;
}

class screenshotGridItem : public QWidget
{
    Q_OBJECT

public:    explicit screenshotGridItem(QWidget *parent = nullptr, QPixmap *screen = nullptr, qint64 hashSum = 0, double similarity = 0.0);
    ~screenshotGridItem();

private:
    Ui::screenshotGridItem *ui;
    QPixmap *screen;
    qint64 hashSum;
    double similarity;
};

#endif // SCREENSHOTGRIDITEM_H
