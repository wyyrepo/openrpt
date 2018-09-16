#ifndef DMATRIXSQUARECONFIG_H
#define DMATRIXSQUARECONFIG_H

#include <QWidget>
#include <QString>
#include "dmatrixpreview.h"

namespace Ui {
    class DMatrixSquareConfig;
}



class DMatrixSquareConfig : public QWidget
{
    Q_OBJECT

public:
    explicit DMatrixSquareConfig(QWidget *parent = 0);
    ~DMatrixSquareConfig();

    QString format();

public slots:
    void setVisible2(bool visible = false);

protected slots:
    virtual void languageChange();

public slots:
    void setCursorValue(int i);

private slots:
    void on__previewButton_clicked();
    void formatRefresh(int i);
    void capacityRefresh(int i);

signals:
    void refreshWidget(int i);

private:

    Ui::DMatrixSquareConfig *ui;
    DMatrixPreview *_preview;

    static QString dimension[];
    static QString capacity[];
};

#endif // DMATRIXSQUARECONFIG_H
