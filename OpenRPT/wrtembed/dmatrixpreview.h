#ifndef DMATRIXPREVIEW_H
#define DMATRIXPREVIEW_H

#include <QWidget>
#include <QDialog>
#include <QPainter>
#include "dmtx.h"

namespace Ui {
    class DMatrixPreview;
}

class DMatrixPreview : public QDialog
{
    Q_OBJECT

public:
    explicit DMatrixPreview(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~DMatrixPreview();
    inline void setFormat(int format) {this->_format=format;}

public slots:
    void generePreview(int format);

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void escapeEvent();

signals:
    bool escapeSignal(bool);

private:
    Ui::DMatrixPreview *ui;
    int _format;
    static QString previewString;

    DmtxEncode     *enc;
    DmtxImage      *img;
};

#endif // DMATRIXPREVIEW_H
