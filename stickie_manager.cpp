#include "stickie_manager.h"
#include "stickie_card.h"

#include <QTextEdit>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QShortcut>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QFileDialog>
#include <QTextStream>
#include <QDate>
#include <QPainter>
#include <QTimer>
#include <QMessageBox>

StickieManager::StickieManager(QObject *parent)
    : QObject(parent)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir().mkpath(configDir + QStringLiteral("/stickies"));
    m_configPath = configDir + QStringLiteral("/stickies/notes.json");

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(300);
    connect(m_saveTimer, &QTimer::timeout, this, &StickieManager::saveNotes);

    setupShortcuts();
    loadNotes();

    if (m_cards.isEmpty())
        createNewCard();

    setupSystemTray();

    connect(qApp, &QApplication::aboutToQuit, this, &StickieManager::saveNotes);
}

StickieManager::~StickieManager()
{
    saveNotes();
}

// ── Shortcuts ──────────────────────────────────────────────────────────────

void StickieManager::setupShortcuts()
{
    auto *newNote = new QShortcut(QKeySequence(QStringLiteral("Ctrl+N")), nullptr);
    connect(newNote, &QShortcut::activated, this, [this]() { createNewCard(); });

    auto *saveAll = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")), nullptr);
    connect(saveAll, &QShortcut::activated, this, &StickieManager::saveNotes);

    auto *closeAllSc = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+W")), nullptr);
    connect(closeAllSc, &QShortcut::activated, this, &StickieManager::hideAll);
}

// ── Create / Close cards ───────────────────────────────────────────────────

void StickieManager::createNewCard(const QString &bgColor)
{
    QJsonObject data;
    if (!bgColor.isEmpty())
        data[QStringLiteral("bg_color")] = bgColor;

    auto *card = new StickieCard(data);

    auto *te = card->findChild<QTextEdit *>();
    connect(te, &QTextEdit::textChanged, this, [this]() {
        m_saveTimer->start();
    });
    te->installEventFilter(this);

    connect(card, &StickieCard::saveRequested, this, &StickieManager::saveNotes);
    connect(card, &StickieCard::moved,   this, [this]() { m_saveTimer->start(); });
    connect(card, &StickieCard::resized, this, [this]() { m_saveTimer->start(); });

    m_cards.append(card);
    card->showWithAnimation();
}

void StickieManager::hideAll()
{
    for (auto *card : m_cards)
        card->hideWithAnimation();
}

// ── Persistence ────────────────────────────────────────────────────────────

void StickieManager::saveNotes()
{
    QJsonArray notes;
    for (auto *card : m_cards)
        notes.append(card->toJson());

    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("notes")]   = notes;

    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void StickieManager::loadNotes()
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    if (root[QStringLiteral("version")].toInt() != 1) return;

    QJsonArray notes = root[QStringLiteral("notes")].toArray();
    for (const auto &val : notes) {
        if (!val.isObject()) continue;
        auto *card = new StickieCard(val.toObject());
        connect(card, &StickieCard::saveRequested, this, &StickieManager::saveNotes);
        connect(card, &StickieCard::moved,   this, [this]() { m_saveTimer->start(); });
    connect(card, &StickieCard::resized, this, [this]() { m_saveTimer->start(); });
        auto *te = card->findChild<QTextEdit *>();
        connect(te, &QTextEdit::textChanged, this, [this]() {
            m_saveTimer->start();
        });
        te->installEventFilter(this);
        m_cards.append(card);
        card->showWithAnimation();
    }
}

// ── Event filter (save on focus out) ───────────────────────────────────────

bool StickieManager::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        // Check if it's a QTextEdit we manage
        if (qobject_cast<QTextEdit *>(obj)) {
            m_saveTimer->stop();
            saveNotes();
        }
    }
    return QObject::eventFilter(obj, event);
}

// ── Export HTML ────────────────────────────────────────────────────────────

void StickieManager::exportToHtml(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << QStringLiteral("<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n");
    out << QStringLiteral("<title>Stickies – Export</title>\n");
    out << QStringLiteral("<style>\n");
    out << QStringLiteral("body { font-family:sans-serif; background:#f5f5f5; padding:30px; }\n");
    out << QStringLiteral("h1 { color:#333; }\n");
    out << QStringLiteral(".note { display:inline-block; width:260px; min-height:240px;\n");
    out << QStringLiteral("  margin:12px; padding:10px; border-radius:8px;\n");
    out << QStringLiteral("  box-shadow:2px 2px 12px rgba(0,0,0,0.15);\n");
    out << QStringLiteral("  vertical-align:top; position:relative; }\n");
    out << QStringLiteral(".note .meta { font-size:11px; color:#888; margin-bottom:6px; }\n");
    out << QStringLiteral("</style>\n</head><body>\n");
    out << QStringLiteral("<h1>Stickies Notes</h1>\n<p>")
        << QDate::currentDate().toString(QStringLiteral("dd/MM/yyyy"))
        << QStringLiteral("</p>\n");

    for (auto *card : m_cards) {
        QJsonObject obj = card->toJson();
        QColor bg = StickieCard::colorMap()
                        .value(obj[QStringLiteral("bg_color")].toString(), QColor("#FFF8E1"));
        out << QStringLiteral("<div class=\"note\" style=\"background-color:")
            << bg.name()
            << QStringLiteral(";\">\n");
        out << QStringLiteral("  <div class=\"meta\">")
            << obj[QStringLiteral("bg_color")].toString()
            << QStringLiteral("</div>\n");
        out << obj[QStringLiteral("text")].toString() << QStringLiteral("\n");
        out << QStringLiteral("</div>\n");
    }

    out << QStringLiteral("</body></html>\n");
    file.close();
}

// ── System Tray ────────────────────────────────────────────────────────────

void StickieManager::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#FFF8E1"));
        p.setPen(QPen(QColor(0, 0, 0, 60), 1));
        p.drawRoundedRect(3, 4, 26, 24, 4, 4);
        p.setPen(QColor(0, 0, 0, 140));
        QFont f = p.font();
        f.setPixelSize(14);
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRect(3, 4, 26, 24), Qt::AlignCenter, QStringLiteral("N"));
    }

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(pix));
    m_trayIcon->setToolTip(QStringLiteral("Stickies"));

    auto *menu = new QMenu();
    menu->addAction(QStringLiteral("Nova Nota"), this, [this]() { createNewCard(); });
    menu->addSeparator();
    menu->addAction(QStringLiteral("Mostrar Todas"), this, [this]() {
        for (auto *c : m_cards) {
            c->show();
            c->raise();
            c->activateWindow();
        }
    });
    menu->addAction(QStringLiteral("Esconder Todas"), this, [this]() {
        for (auto *c : m_cards) c->hideWithAnimation();
    });
    menu->addSeparator();
    menu->addAction(QStringLiteral("Exportar HTML…"), this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            nullptr, QStringLiteral("Exportar Notas"),
            QStringLiteral("stickies.html"), QStringLiteral("HTML (*.html)"));
        if (!path.isEmpty()) exportToHtml(path);
    });
    menu->addSeparator();
    menu->addAction(QStringLiteral("Sobre"), this, [this]() {
        QMessageBox box;
        box.setWindowTitle(QStringLiteral("Sobre o Stickies"));
        box.setText(QStringLiteral("<b>Stickies</b>"));
        box.setInformativeText(
            QStringLiteral(
                "Feito por <b>Povman</b><br><br>"
                "Licença: MIT<br><br>"
                "Website: <a href='https://fabiomoraes.sisdigital.com.br'>https://fabiomoraes.sisdigital.com.br</a><br><br>"
                "Arquivo de notas:<br><code>%1</code>"
            ).arg(m_configPath)
        );
        box.setIcon(QMessageBox::Information);
        box.exec();
    });
    menu->addSeparator();
    menu->addAction(QStringLiteral("Sair"), qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(menu);
    m_trayIcon->show();
}
