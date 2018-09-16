#ifndef DMATRIXRECTCONFIG_H
#define DMATRIXRECTCONFIG_H

#include <QWidget>
#include <QComboBox>
#include <QString>
#include "dmatrixpreview.h"

namespace Ui {
    class DMatrixRectConfig;
}

//dans le fichier dmtx.h l'enumaration des datamatrix carré commence à 24
class DMatrixRectConfig : public QWidget
{
    Q_OBJECT

public:
    explicit DMatrixRectConfig(QWidget *parent = 0);
    ~DMatrixRectConfig();

    QString format();

public slots:
    void setVisible2(bool visible = false);
    void setIndexValue(int index);

private slots:
    void on__previewButton_clicked();
    void capacityRefresh(int i);
    void indexType(int index);

signals:
    void signalType(int index);

private:

    Ui::DMatrixRectConfig *ui;
    DMatrixPreview *_preview;

    //capacité à titre indicatif, possible erreur d'écriture
    static QString capacite[];
};

#endif // DMATRIXRECTCONFIG_H
