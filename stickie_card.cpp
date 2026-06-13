#include "stickie_card.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QJsonObject>

#include <QCloseEvent>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QWindow>

static const QMap<QString, QColor> &colorMap()
{
    static const QMap<QString, QColor> map = {
        {QStringLiteral("yellow"),      QColor("#FFF8E1")},
        {QStringLiteral("blue"),        QColor("#BBDEFB")},
        {QStringLiteral("green"),       QColor("#C8E6C9")},
        {QStringLiteral("red"),         QColor("#FFCDD2")},
        {QStringLiteral("white"),       QColor("#FFFFFF")},
        {QStringLiteral("transparent"), QColor(0, 0, 0, 0)},
    };
    return map;
}

const QMap<QString, QColor> &StickieCard::colorMap() { return ::colorMap(); }

StickieCard::StickieCard(const QJsonObject &data, QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    setMinimumSize(320, 180);

    m_id = data.contains(QStringLiteral("id"))
               ? data[QStringLiteral("id")].toString()
               : QUuid::createUuid().toString(QUuid::WithoutBraces);

    m_bgColorKey = data.contains(QStringLiteral("bg_color"))
                       ? data[QStringLiteral("bg_color")].toString()
                       : QStringLiteral("yellow");

    setupUI();

    if (data.contains(QStringLiteral("text")))
        m_textEdit->setHtml(data[QStringLiteral("text")].toString());

    int x = data.contains(QStringLiteral("x")) ? data[QStringLiteral("x")].toInt() : 300;
    int y = data.contains(QStringLiteral("y")) ? data[QStringLiteral("y")].toInt() : 200;
    int w = data.contains(QStringLiteral("width"))  ? data[QStringLiteral("width")].toInt()  : 360;
    int h = data.contains(QStringLiteral("height")) ? data[QStringLiteral("height")].toInt() : 280;
    setGeometry(x, y, w, h);

    applyBackground();
}

StickieCard::~StickieCard() = default;

// ── UI Setup ───────────────────────────────────────────────────────────────

void StickieCard::setupUI()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(kShadowMargin, kShadowMargin, kShadowMargin, kShadowMargin);
    outer->setSpacing(0);

    m_container = new QWidget(this);
    m_container->setObjectName(QStringLiteral("cardContainer"));

    auto *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20.0);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 110));
    m_container->setGraphicsEffect(shadow);

    auto *cl = new QVBoxLayout(m_container);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    m_toolbar = new QWidget();
    m_toolbar->setObjectName(QStringLiteral("toolbar"));
    m_toolbar->setFixedHeight(28);
    m_toolbar->installEventFilter(this);

    auto *tb = new QHBoxLayout(m_toolbar);
    tb->setContentsMargins(4, 0, 4, 0);
    tb->setSpacing(2);

    auto makeBtn = [&](const QString &text, const QString &tip, const QString &extraStyle) -> QPushButton* {
        auto *b = new QPushButton(text);
        b->setFixedSize(22, 20);
        b->setToolTip(tip);
        b->setCursor(Qt::ArrowCursor);
        b->setFocusPolicy(Qt::NoFocus);
        b->setCheckable(true);
        b->setStyleSheet(
            QStringLiteral("QPushButton { background:transparent; border:1px solid transparent;"
                           " border-radius:3px; font-size:11px; color:#000000; %1 }"
                           "QPushButton:hover { background:rgba(0,0,0,0.1);"
                           " border:1px solid rgba(0,0,0,0.2); color:#000000; }"
                           "QPushButton:checked { background:rgba(0,0,0,0.18);"
                           " border:1px solid rgba(0,0,0,0.3); color:#000000; }")
                .arg(extraStyle));
        return b;
    };

    m_btnBold      = makeBtn(QStringLiteral("B"), QStringLiteral("Negrito"),       QStringLiteral("font-weight:bold;"));
    m_btnItalic    = makeBtn(QStringLiteral("I"), QStringLiteral("Itálico"),       QStringLiteral("font-style:italic;"));
    m_btnUnderline = makeBtn(QStringLiteral("U"), QStringLiteral("Sublinhado"),    QStringLiteral("text-decoration:underline;"));
    m_btnStrike    = makeBtn(QStringLiteral("S"), QStringLiteral("Tachado"),       QStringLiteral("text-decoration:line-through;"));

    tb->addWidget(m_btnBold);
    tb->addWidget(m_btnItalic);
    tb->addWidget(m_btnUnderline);
    tb->addWidget(m_btnStrike);

    // Font color picker
    auto updateFontColorIcon = [this]() {
        QColor c = m_textEdit
            ? m_textEdit->currentCharFormat().foreground().color()
            : QColor(Qt::black);
        QPixmap pm(20, 18);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        QFont f = p.font();
        f.setPixelSize(12);
        f.setBold(true);
        p.setFont(f);
        p.setPen(c);
        p.drawText(QRect(0, 0, 20, 16), Qt::AlignCenter, QStringLiteral("A"));
        p.setPen(QPen(c, 2));
        p.drawLine(4, 15, 16, 15);
        p.end();
        m_btnFontColor->setIcon(QIcon(pm));
        m_btnFontColor->setIconSize(QSize(20, 18));
    };

    m_btnFontColor = new QPushButton();
    m_btnFontColor->setFixedSize(22, 20);
    m_btnFontColor->setToolTip(QStringLiteral("Cor da fonte"));
    m_btnFontColor->setCursor(Qt::ArrowCursor);
    m_btnFontColor->setFocusPolicy(Qt::NoFocus);
    m_btnFontColor->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid transparent; border-radius:3px; color:#000000; }"
        "QPushButton:hover { background:rgba(0,0,0,0.1); border:1px solid rgba(0,0,0,0.2); color:#000000; }"));
    updateFontColorIcon();
    connect(m_btnFontColor, &QPushButton::clicked, this, [this, updateFontColorIcon]() {
        QColor current = m_textEdit->currentCharFormat().foreground().color();
        QColor chosen = QColorDialog::getColor(current, this, QStringLiteral("Cor da fonte"));
        if (chosen.isValid()) {
            QTextCharFormat f;
            f.setForeground(chosen);
            mergeFormatOnSelection(f);
            updateFontColorIcon();
        }
    });
    tb->addWidget(m_btnFontColor);

    m_btnDecreaseFont = new QPushButton(QStringLiteral("A−"));
    m_btnDecreaseFont->setFixedSize(22, 20);
    m_btnDecreaseFont->setToolTip(QStringLiteral("Diminuir fonte"));
    m_btnDecreaseFont->setCursor(Qt::ArrowCursor);
    m_btnDecreaseFont->setFocusPolicy(Qt::NoFocus);
    m_btnDecreaseFont->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid transparent; border-radius:3px;"
        " color:#000000; font-size:11px; }"
        "QPushButton:hover { background:rgba(0,0,0,0.1); border:1px solid rgba(0,0,0,0.2); }"));
    connect(m_btnDecreaseFont, &QPushButton::clicked, this, [this]() {
        QTextCharFormat f;
        qreal sz = m_textEdit->currentCharFormat().fontPointSize();
        if (sz < 1) sz = m_textEdit->font().pointSizeF();
        if (sz < 1) sz = 13;
        f.setFontPointSize(qMax(6.0, sz - 2));
        mergeFormatOnSelection(f);
    });
    tb->addWidget(m_btnDecreaseFont);

    m_btnIncreaseFont = new QPushButton(QStringLiteral("A+"));
    m_btnIncreaseFont->setFixedSize(22, 20);
    m_btnIncreaseFont->setToolTip(QStringLiteral("Aumentar fonte"));
    m_btnIncreaseFont->setCursor(Qt::ArrowCursor);
    m_btnIncreaseFont->setFocusPolicy(Qt::NoFocus);
    m_btnIncreaseFont->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid transparent; border-radius:3px;"
        " color:#000000; font-weight:bold; font-size:13px; }"
        "QPushButton:hover { background:rgba(0,0,0,0.1); border:1px solid rgba(0,0,0,0.2); }"));
    connect(m_btnIncreaseFont, &QPushButton::clicked, this, [this]() {
        QTextCharFormat f;
        qreal sz = m_textEdit->currentCharFormat().fontPointSize();
        if (sz < 1) sz = m_textEdit->font().pointSizeF();
        if (sz < 1) sz = 13;
        f.setFontPointSize(qMin(72.0, sz + 2));
        mergeFormatOnSelection(f);
    });
    tb->addWidget(m_btnIncreaseFont);

    auto *sep = new QWidget();
    sep->setFixedWidth(1);
    sep->setFixedHeight(18);
    sep->setStyleSheet(QStringLiteral("background:rgba(0,0,0,0.12);"));
    tb->addWidget(sep);

    // Color swatches
    for (const auto &key : {QStringLiteral("yellow"),  QStringLiteral("blue"),
                            QStringLiteral("green"),   QStringLiteral("red"),
                            QStringLiteral("white"),   QStringLiteral("transparent")}) {
        auto *cb = new QPushButton();
        cb->setFixedSize(16, 16);
        cb->setToolTip(key);
        cb->setCursor(Qt::ArrowCursor);
        cb->setFocusPolicy(Qt::NoFocus);
        cb->setProperty("colorKey", key);

        QColor c = ::colorMap().value(key);
        if (key == QStringLiteral("transparent")) {
            QPixmap pm(16, 16);
            pm.fill(Qt::white);
            { QPainter p(&pm); p.setPen(QColor(180,180,180)); p.drawLine(0,0,16,16); }
            cb->setIcon(QIcon(pm));
            cb->setIconSize(QSize(16, 16));
            cb->setStyleSheet(QStringLiteral(
                "QPushButton { border:1px solid rgba(0,0,0,0.25); border-radius:2px; }"
                "QPushButton:hover { border:2px solid rgba(0,0,0,0.5); }"));
        } else {
            cb->setStyleSheet(
                QStringLiteral("QPushButton { background-color:%1; border:1px solid rgba(0,0,0,0.2);"
                               " border-radius:2px; }"
                               "QPushButton:hover { border:2px solid rgba(0,0,0,0.5); }")
                    .arg(c.name()));
        }

        connect(cb, &QPushButton::clicked, this, [this, key]() { setBackgroundColor(key); });
        tb->addWidget(cb);
    }

    tb->addStretch();

    // ── Drag handle ────────────────────────────────────────────────────────
    m_btnDrag = new QPushButton();
    m_btnDrag->setFixedSize(22, 20);
    m_btnDrag->setToolTip(QStringLiteral("Mover nota"));
    m_btnDrag->setCursor(Qt::SizeAllCursor);
    m_btnDrag->setFocusPolicy(Qt::NoFocus);
    m_btnDrag->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid transparent; border-radius:3px; }"
        "QPushButton:hover { background:rgba(0,0,0,0.1); border:1px solid rgba(0,0,0,0.2); }"
        "QPushButton:pressed { background:rgba(0,0,0,0.15); }"));
    {
        QPixmap pm(20, 20);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(70, 70, 80, 170));
        p.setPen(Qt::NoPen);
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 2; ++col)
                p.drawEllipse(6 + col * 5, 4 + row * 5, 3, 3);
        p.end();
        m_btnDrag->setIcon(QIcon(pm));
        m_btnDrag->setIconSize(QSize(20, 20));
    }
    m_btnDrag->installEventFilter(this);
    tb->addWidget(m_btnDrag);

    m_btnSave = new QPushButton();
    m_btnSave->setFixedSize(22, 20);
    m_btnSave->setToolTip(QStringLiteral("Salvar nota"));
    m_btnSave->setCursor(Qt::ArrowCursor);
    m_btnSave->setFocusPolicy(Qt::NoFocus);
    m_btnSave->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid transparent; border-radius:3px; }"
        "QPushButton:hover { background:rgba(0,0,0,0.1); border:1px solid rgba(0,0,0,0.2); }"
        "QPushButton:pressed { background:rgba(0,0,0,0.15); }"));
    {
        QPixmap pm(32, 32);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);

        // Corpo principal — azul
        p.setPen(QPen(QColor(18, 64, 148), 1.5));
        p.setBrush(QColor(52, 118, 210));
        p.drawRoundedRect(1, 1, 29, 29, 3, 3);

        // Etiqueta (área branco-azulada no topo)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(195, 220, 255));
        p.drawRoundedRect(4, 3, 17, 13, 1, 1);

        // Aba de proteção contra escrita (quadrado escuro, canto sup. direito)
        p.setBrush(QColor(30, 80, 175));
        p.drawRoundedRect(22, 3, 6, 8, 1, 1);

        // Slot/orifício da aba de proteção
        p.setBrush(QColor(18, 64, 148));
        p.drawRect(23, 5, 4, 4);

        // Proteção metálica inferior (shutter)
        p.setPen(QPen(QColor(18, 64, 148), 1));
        p.setBrush(QColor(165, 178, 200));
        p.drawRect(2, 20, 27, 9);

        // Linha divisória corpo/shutter
        p.setPen(QPen(QColor(18, 64, 148), 1.2));
        p.drawLine(1, 20, 30, 20);

        // Janela do shutter (orifício do disco)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(38, 42, 58));
        p.drawRoundedRect(9, 22, 13, 5, 1, 1);

        // Reflexo metálico no shutter
        p.setBrush(QColor(210, 218, 230));
        p.drawRect(2, 20, 27, 2);

        p.end();
        m_btnSave->setIcon(QIcon(pm));
        m_btnSave->setIconSize(QSize(20, 20));
    }
    connect(m_btnSave, &QPushButton::clicked, this, &StickieCard::saveRequested);
    tb->addWidget(m_btnSave);

    auto *closeBtn = new QPushButton(QStringLiteral("\u2715"));
    closeBtn->setFixedSize(22, 20);
    closeBtn->setToolTip(QStringLiteral("Fechar"));
    closeBtn->setCursor(Qt::ArrowCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; border:1px solid transparent; border-radius:3px;"
        " color:rgba(0,0,0,0.45); font-size:14px; }"
        "QPushButton:hover { background:rgba(200,0,0,0.25); border:1px solid rgba(200,0,0,0.4);"
        " color:rgba(0,0,0,0.8); }"));
    connect(closeBtn, &QPushButton::clicked, this, &StickieCard::hideWithAnimation);
    tb->addWidget(closeBtn);

    cl->addWidget(m_toolbar);

    // ── Text edit ──────────────────────────────────────────────────────────
    m_textEdit = new QTextEdit();
    m_textEdit->setFrameShape(QFrame::NoFrame);
    m_textEdit->setTabChangesFocus(false);
    m_textEdit->setPlaceholderText(QStringLiteral("Digite sua nota…"));
    QTextCharFormat defaultFmt;
    defaultFmt.setForeground(Qt::black);
    m_textEdit->setCurrentCharFormat(defaultFmt);
    m_textEdit->setStyleSheet(QStringLiteral(
        "QTextEdit { background:transparent; border:none; padding:4px 8px; font-size:13px; color:#000000; }"
        "QScrollBar:vertical { width:6px; background:transparent; }"
        "QScrollBar::handle:vertical { background:rgba(0,0,0,0.15); border-radius:3px; min-height:20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0px; }"));
    cl->addWidget(m_textEdit);
    m_textEdit->installEventFilter(this);

    outer->addWidget(m_container);

    // ── Connect style buttons ──────────────────────────────────────────────
    connect(m_btnBold, &QPushButton::toggled, this, [this](bool checked) {
        QTextCharFormat f;
        f.setFontWeight(checked ? QFont::Bold : QFont::Normal);
        mergeFormatOnSelection(f);
    });
    connect(m_btnItalic, &QPushButton::toggled, this, [this](bool checked) {
        QTextCharFormat f;
        f.setFontItalic(checked);
        mergeFormatOnSelection(f);
    });
    connect(m_btnUnderline, &QPushButton::toggled, this, [this](bool checked) {
        QTextCharFormat f;
        f.setFontUnderline(checked);
        mergeFormatOnSelection(f);
    });
    connect(m_btnStrike, &QPushButton::toggled, this, [this](bool checked) {
        QTextCharFormat f;
        f.setFontStrikeOut(checked);
        mergeFormatOnSelection(f);
    });

    connect(m_textEdit, &QTextEdit::cursorPositionChanged, this, [this, updateFontColorIcon]() {
        QFont f = m_textEdit->currentFont();
        m_btnBold->setChecked(f.bold());
        m_btnItalic->setChecked(f.italic());
        m_btnUnderline->setChecked(m_textEdit->currentCharFormat().fontUnderline());
        m_btnStrike->setChecked(m_textEdit->currentCharFormat().fontStrikeOut());
        updateFontColorIcon();
    });
}

// ── Background ─────────────────────────────────────────────────────────────

void StickieCard::applyBackground()
{
    QColor bg = ::colorMap().value(m_bgColorKey, QColor("#FFF8E1"));

    if (m_bgColorKey == QStringLiteral("transparent")) {
        m_container->setStyleSheet(QStringLiteral(
            "#cardContainer { background:transparent; border-radius:8px;"
            " border:1px solid rgba(0,0,0,0.08); }"));
        m_toolbar->setStyleSheet(QStringLiteral(
            "#toolbar { background:rgba(255,255,255,210);"
            " border-top-left-radius:8px; border-top-right-radius:8px; }"));
    } else {
        m_container->setStyleSheet(
            QStringLiteral("#cardContainer { background-color:%1; border-radius:8px;"
                           " border:1px solid rgba(0,0,0,0.07); }")
                .arg(bg.name()));
        m_toolbar->setStyleSheet(
            QStringLiteral("#toolbar { background-color:%1;"
                           " border-top-left-radius:8px; border-top-right-radius:8px; }")
                .arg(bg.darker(104).name()));
    }
}

void StickieCard::setBackgroundColor(const QString &colorKey)
{
    m_bgColorKey = colorKey;
    applyBackground();
}

QJsonObject StickieCard::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")]       = m_id;
    obj[QStringLiteral("text")]     = m_textEdit->toHtml();
    obj[QStringLiteral("x")]        = geometry().x();
    obj[QStringLiteral("y")]        = geometry().y();
    obj[QStringLiteral("width")]    = geometry().width();
    obj[QStringLiteral("height")]   = geometry().height();
    obj[QStringLiteral("bg_color")] = m_bgColorKey;
    return obj;
}

// ── Mouse ──────────────────────────────────────────────────────────────────

StickieCard::ResizeEdge StickieCard::getResizeEdge(const QPoint &pos) const
{
    const int x = pos.x(), y = pos.y();
    const int w = width(), h = height();
    const bool left   = x <= kResizeMargin;
    const bool right  = x >= w - kResizeMargin;
    const bool top    = y <= kResizeMargin;
    const bool bottom = y >= h - kResizeMargin;

    if      (top  && left)  return TopLeft;
    else if (top  && right) return TopRight;
    else if (bottom && left)  return BottomLeft;
    else if (bottom && right) return BottomRight;
    else if (left)           return Left;
    else if (right)          return Right;
    else if (top)            return Top;
    else if (bottom)         return Bottom;
    return None;
}

void StickieCard::updateCursorForEdge(ResizeEdge edge)
{
    switch (edge) {
    case TopLeft:
    case BottomRight:  setCursor(Qt::SizeFDiagCursor); break;
    case TopRight:
    case BottomLeft:   setCursor(Qt::SizeBDiagCursor); break;
    case Left:
    case Right:        setCursor(Qt::SizeHorCursor);   break;
    case Top:
    case Bottom:       setCursor(Qt::SizeVerCursor);   break;
    default:           setCursor(Qt::ArrowCursor);     break;
    }
}

void StickieCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        ResizeEdge edge = getResizeEdge(event->position().toPoint());
        if (edge != None && windowHandle()) {
            static const Qt::Edges edgeMap[] = {
                Qt::Edges{},
                Qt::LeftEdge,
                Qt::RightEdge,
                Qt::TopEdge,
                Qt::BottomEdge,
                Qt::TopEdge | Qt::LeftEdge,
                Qt::TopEdge | Qt::RightEdge,
                Qt::BottomEdge | Qt::LeftEdge,
                Qt::BottomEdge | Qt::RightEdge,
            };
            windowHandle()->startSystemResize(edgeMap[edge]);
            event->accept();
            return;
        }
        if (windowHandle())
            windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void StickieCard::mouseMoveEvent(QMouseEvent *event)
{
    updateCursorForEdge(getResizeEdge(event->position().toPoint()));
    QWidget::mouseMoveEvent(event);
}

void StickieCard::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}

void StickieCard::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    emit moved();
}

void StickieCard::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    emit resized();
}

// ── Toolbar event filter (drag support) ────────────────────────────────────

bool StickieCard::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_toolbar || obj == m_btnDrag) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && windowHandle()) {
                windowHandle()->startSystemMove();
                return true;
            }
        }
    } else if (obj == m_textEdit) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                m_potentialDrag = true;
                m_potentialDragStart = me->globalPosition().toPoint();
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (m_potentialDrag) {
                if ((me->globalPosition().toPoint() - m_potentialDragStart).manhattanLength() > kDragThreshold) {
                    m_potentialDrag = false;
                    m_dragging = true;
                    QTextCursor c = m_textEdit->textCursor();
                    c.clearSelection();
                    m_textEdit->setTextCursor(c);
                    if (windowHandle())
                        windowHandle()->startSystemMove();
                }
            }
            if (m_dragging)
                return true;
            break;
        }
        case QEvent::MouseButtonRelease: {
            m_potentialDrag = false;
            if (m_dragging) { m_dragging = false; return true; }
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ── Animation ──────────────────────────────────────────────────────────────

void StickieCard::showWithAnimation()
{
    show();
    raise();
}

void StickieCard::hideWithAnimation()
{
    if (!isVisible()) return;
    hide();
}

void StickieCard::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hideWithAnimation();
}

void StickieCard::deletePermanently()
{
    deleteLater();
}

// ── Text formatting ────────────────────────────────────────────────────────

void StickieCard::mergeFormatOnSelection(const QTextCharFormat &fmt)
{
    QTextCursor cursor = m_textEdit->textCursor();
    if (!cursor.hasSelection())
        m_textEdit->mergeCurrentCharFormat(fmt);
    else
        cursor.mergeCharFormat(fmt);
    m_textEdit->setFocus();
}

// ── Paint ──────────────────────────────────────────────────────────────────

void StickieCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor bg = ::colorMap().value(m_bgColorKey);
    if (bg.alpha() > 0) {
        painter.setBrush(bg);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect().adjusted(kShadowMargin + 1, kShadowMargin + 1,
                                                -(kShadowMargin + 1), -(kShadowMargin + 1)), 8, 8);
    }
}
