#ifndef STICKIE_CARD_H
#define STICKIE_CARD_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <QPoint>
#include <QSize>
#include <QColor>
#include <QMap>

class QGraphicsDropShadowEffect;

class StickieCard : public QWidget
{
    Q_OBJECT

public:
    explicit StickieCard(const QJsonObject &data = QJsonObject(), QWidget *parent = nullptr);
    ~StickieCard() override;

    QString id() const { return m_id; }
    QJsonObject toJson() const;
    void setBackgroundColor(const QString &colorKey);
    static const QMap<QString, QColor> &colorMap();

signals:
    void saveRequested();
    void moved();
    void resized();

public slots:
    void hideWithAnimation();
    void showWithAnimation();
    void deletePermanently();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    enum ResizeEdge { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };

    void setupUI();
    void applyBackground();
    ResizeEdge getResizeEdge(const QPoint &pos) const;
    void updateCursorForEdge(ResizeEdge edge);
    void mergeFormatOnSelection(const QTextCharFormat &fmt);

    QString m_id;
    QString m_bgColorKey;
    QTextEdit *m_textEdit = nullptr;
    QWidget *m_container = nullptr;
    QWidget *m_toolbar = nullptr;
    QPushButton *m_btnBold = nullptr;
    QPushButton *m_btnItalic = nullptr;
    QPushButton *m_btnUnderline = nullptr;
    QPushButton *m_btnStrike = nullptr;
    QPushButton *m_btnFontColor = nullptr;
    QPushButton *m_btnIncreaseFont = nullptr;
    QPushButton *m_btnDecreaseFont = nullptr;
    QPushButton *m_btnDrag = nullptr;
    QPushButton *m_btnSave = nullptr;

    bool m_dragging = false;

    bool m_potentialDrag = false;
    QPoint m_potentialDragStart;

    static constexpr int kShadowMargin = 22;
    static constexpr int kResizeMargin = 6;
    static constexpr int kDragThreshold = 5;
};

#endif
