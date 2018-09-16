#include "dmatrixpreview.h"
#include "ui_dmatrixpreview.h"
#include <cmath>

#include <QDebug>

QString DMatrixPreview::previewString = "012345";

DMatrixPreview::DMatrixPreview(QWidget *parent, Qt::WindowFlags fl) :
    QDialog(parent,fl),
    ui(new Ui::DMatrixPreview)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Preview"));

    connect(this,SIGNAL(rejected()),this,SLOT(escapeEvent()));

    this->enc = NULL;
    this->img = NULL;
}

DMatrixPreview::~DMatrixPreview()
{
    delete ui;
    dmtxEncodeDestroy(&enc);
    dmtxImageDestroy(&img);
}

void DMatrixPreview::generePreview(int format)
{
    this->_format = format;
    if(this->enc != NULL)
    {
        dmtxEncodeDestroy(&enc);
        enc = NULL;
    }
    if(this->img != NULL)
    {
        dmtxImageDestroy(&img);
        img = NULL;
    }

    size_t          width, height, bytesPerPixel;
    unsigned char  *pxl;
    /* 1) ENCODE a new Data Matrix barcode image (in memory only) */
    enc = dmtxEncodeCreate();

    //voir l'enumération DmtxSymbolSize dans dmtx.h pour plus de détails
    enc->sizeIdxRequest = this->_format;
    enc->marginSize = 0;
    enc->moduleSize = 1;

    //assert(enc != NULL);
    dmtxEncodeDataMatrix(enc, this->previewString.size(), (unsigned char*)this->previewString.toStdString().c_str());

    /* 2) COPY the new image data before releasing encoding memory */

    width = dmtxImageGetProp(enc->image, DmtxPropWidth);
    height = dmtxImageGetProp(enc->image, DmtxPropHeight);
    bytesPerPixel = dmtxImageGetProp(enc->image, DmtxPropBytesPerPixel);

    pxl = (unsigned char *)malloc(width * height * bytesPerPixel);
    //assert(pxl != NULL);
    memcpy(pxl, enc->image->pxl, width * height * bytesPerPixel);

    dmtxEncodeDestroy(&enc);

    /* 3) DECODE the Data Matrix barcode from the copied image */
    img = dmtxImageCreate(pxl, width, height, DmtxPack24bppRGB);

    this->repaint();
}

void DMatrixPreview::paintEvent(QPaintEvent *)
{
    QPainter dessin(this);
    dessin.setRenderHint(QPainter::Antialiasing,true);

    double heigth=(double)this->height();
    double width=(double)this->width();

    //length of a square
    double pas = std::min(heigth/(img->height),width/(img->width));

    dessin.setBrush(Qt::white);
    dessin.setPen(Qt::white);
    dessin.drawRect(0,0,width,heigth);

    //center of Widget
    double Yc = heigth/2;
    double Xc = width/2;

    double Xo = Xc - (img->width*pas)/2;
    double Yo = Yc + (img->height*pas)/2 - pas;

    int valeur = 0;
    //for(int y = img->height - 1; y >= 0; y--)
    for(int y = 0; y < img->height; y++)
    {
            for(int x = 0; x < img->width; x++)
            {
                    dmtxImageGetPixelValue(img,x,y,0,&valeur);

                    if(valeur < 128)
                    {
                        dessin.setBrush(Qt::black);
                        dessin.setPen(Qt::black);
                            dessin.drawRect(Xo + x*pas,
                                            Yo - y*pas,
                                            pas,
                                            pas);
                    }
            }
    }
}

void DMatrixPreview::escapeEvent()
{
    emit this->escapeSignal(false);
}
