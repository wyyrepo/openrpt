#include "dmatrixsquareconfig.h"
#include "ui_dmatrixsquareconfig.h"

DMatrixSquareConfig::DMatrixSquareConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DMatrixSquareConfig)
{
    ui->setupUi(this);

    this->_preview = new DMatrixPreview(this);
    this->_preview->generePreview(this->ui->_Format->value());
    this->_preview->setVisible(false);

    connect(this->ui->_Format,  SIGNAL(sliderMoved(int)),   this,           SLOT(capacityRefresh(int)));
    connect(this->ui->_Format,  SIGNAL(sliderMoved(int)),   this,           SLOT(formatRefresh(int)));
    connect(this->ui->_Format,  SIGNAL(sliderMoved(int)),   this->_preview, SLOT(generePreview(int)));

    connect(this,               SIGNAL(refreshWidget(int)), this,           SLOT(capacityRefresh(int)));
    connect(this,               SIGNAL(refreshWidget(int)), this,           SLOT(formatRefresh(int)));
    connect(this,               SIGNAL(refreshWidget(int)), this->_preview, SLOT(generePreview(int)));

    connect(this->_preview,     SIGNAL(escapeSignal(bool)), this->ui->_previewButton,   SLOT(setChecked(bool)));
}

DMatrixSquareConfig::~DMatrixSquareConfig()
{
    delete ui;
    delete this->_preview;
}

void DMatrixSquareConfig::languageChange()
{
    this->ui->retranslateUi(this);
}

QString DMatrixSquareConfig::dimension[] =  {   "10x10",    "12x12",    "14x14",    "16x16",    "18x18",    "20x20",    "22x22",    "24x24",    "26x26",    "32x32",    "36x36",
                                                "40x40",    "44x44",    "48x48",    "52x52",    "64x64",    "72x72",    "80x80",    "88x88",    "96x96",    "104x104",  "120x120",
                                                "132x132",  "144x144"};
QString DMatrixSquareConfig::capacity[] =   {   "6/3",      "10/6",     "16/10",    "24/16",    "36/25",    "44/31",    "60/43",    "72/52",    "88/64",    "124/91",   "172/127",
                                                "228/169",  "288/214",  "348/259",  "408/304",  "560/418",  "736/550",  "912/682",  "1152/862", "1392/1042","1632/1222","2100/1573",
                                                "2608/1954","3116/2335"};

QString DMatrixSquareConfig::format()
{
    QString out = "datamatrix_";
    out.append(QString::number(this->ui->_Format->value()));
    return out;
}

void DMatrixSquareConfig::formatRefresh(int i)
{
    this->ui->_LFormat->setText(DMatrixSquareConfig::dimension[i]);
}

void DMatrixSquareConfig::capacityRefresh(int i)
{
    this->ui->_LCapacite->setText(DMatrixSquareConfig::capacity[i]);
}

void DMatrixSquareConfig::setVisible2(bool visible)
{
    this->setVisible(visible);

    if(this->_preview->isVisible())
    {
        this->_preview->setVisible(visible);
    }
}

void DMatrixSquareConfig::on__previewButton_clicked()
{
    if(this->ui->_previewButton->isChecked())
    {
        this->_preview->setVisible(true);
    }
    else
    {
        this->_preview->setVisible(false);
    }
}

void DMatrixSquareConfig::setCursorValue(int i)
{
    this->ui->_Format->setSliderPosition(i);
    //this->ui->_Format->setValue(i);
    emit this->refreshWidget(i);
}
