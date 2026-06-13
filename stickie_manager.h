#ifndef STICKIE_MANAGER_H
#define STICKIE_MANAGER_H

#include <QObject>
#include <QList>
#include <QString>

class StickieCard;
class QSystemTrayIcon;
class QTimer;

class StickieManager : public QObject
{
    Q_OBJECT
public:
    explicit StickieManager(QObject *parent = nullptr);
    ~StickieManager() override;

    void createNewCard(const QString &bgColor = QString());
    void exportToHtml(const QString &filePath);
    void exportToMarkdown(const QString &filePath);

public slots:
    void saveNotes();
    void hideAll();

private:
    void loadNotes();
    bool tryLoadFrom(const QString &path);
    void setupSystemTray();
    void setupShortcuts();
    void deleteCard(StickieCard *card);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QList<StickieCard *> m_cards;
    QString m_configPath;
    QSystemTrayIcon *m_trayIcon  = nullptr;
    QTimer          *m_saveTimer = nullptr;
};

#endif
