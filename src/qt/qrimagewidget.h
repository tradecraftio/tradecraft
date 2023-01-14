// Copyright (c) 2011-2020 The Bitcoin Core developers
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef BITCOIN_QT_QRIMAGEWIDGET_H
#define BITCOIN_QT_QRIMAGEWIDGET_H

#include <QImage>
#include <QLabel>

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* Size of exported QR Code image */
static constexpr int QR_IMAGE_SIZE = 300;
static constexpr int QR_IMAGE_TEXT_MARGIN = 10;
static constexpr int QR_IMAGE_MARGIN = 2 * QR_IMAGE_TEXT_MARGIN;

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

/* Label widget for QR code. This image can be dragged, dropped, copied and saved
 * to disk.
 */
class QRImageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit QRImageWidget(QWidget *parent = nullptr);
    bool setQR(const QString& data, const QString& text = "");
    QImage exportImage();

public Q_SLOTS:
    void saveImage();
    void copyImage();

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QMenu *contextMenu;
};

#endif // BITCOIN_QT_QRIMAGEWIDGET_H
