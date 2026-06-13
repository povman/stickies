#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QStringList>

#ifdef HAVE_HUNSPELL
class Hunspell;
#endif

class SpellHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit SpellHighlighter(QTextDocument *parent = nullptr);
    ~SpellHighlighter() override;

    bool        isLoaded()              const;
    bool        isCorrect(const QString &word) const;
    QStringList suggestions(const QString &word) const;

protected:
    void highlightBlock(const QString &text) override;

private:
    void loadDictionary();

#ifdef HAVE_HUNSPELL
    Hunspell *m_hunspell = nullptr;
#endif
    QTextCharFormat m_errorFmt;
};
