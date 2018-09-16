#include "dbarcodeconfig.h"
#include "ui_dbarcodeconfig.h"

DBarcodeConfig::DBarcodeConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DBarcodeConfig)
{
    ui->setupUi(this);
}

DBarcodeConfig::~DBarcodeConfig()
{
    delete ui;
}

void DBarcodeConfig::setSliderMaxValue(int value)
{
    this->ui->_SliderMaxVal->setValue(value);
}

int DBarcodeConfig::getSliderMaxValue()
{
    return this->ui->_SliderMaxVal->value();
}
