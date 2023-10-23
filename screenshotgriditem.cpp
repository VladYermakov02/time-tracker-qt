#include "screenshotgriditem.h"
#include "./ui_screenshotgriditem.h"

screenshotGridItem::screenshotGridItem(QWidget *parent, QPixmap *screen, qint64 hashSum, double similarity) :
    QWidget(parent),
    ui(new Ui::screenshotGridItem)
{
    ui->setupUi(this);
    this->screen = screen;
    this->hashSum = hashSum;
    this->similarity = similarity;
}

screenshotGridItem::~screenshotGridItem()
{
    delete ui;
}
