#include "spell_highlighter.h"

#ifdef HAVE_HUNSPELL
#  include <hunspell/hunspell.hxx>
#endif

#include <QRegularExpression>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QStringEncoder>
#include <QStringDecoder>

#ifdef HAVE_HUNSPELL
// Retorna o encoding declarado no .aff, sem BOM e sem espaços
static QByteArray cleanDictEncoding(Hunspell *h)
{
    QByteArray enc = QByteArray(h->get_dic_encoding()).trimmed();
    if (enc.startsWith("\xEF\xBB\xBF"))   // strip UTF-8 BOM
        enc = enc.mid(3);
    if (enc.isEmpty())
        enc = "UTF-8";
    return enc;
}

static QByteArray encodeWord(const QString &word, const QByteArray &enc)
{
    QStringEncoder encoder(enc.constData());
    return encoder.isValid() ? encoder.encode(word) : word.toUtf8();
}

static QString decodeWord(const std::string &s, const QByteArray &enc)
{
    QStringDecoder decoder(enc.constData());
    return decoder.isValid()
        ? decoder.decode(QByteArray(s.c_str(), (qsizetype)s.size()))
        : QString::fromUtf8(s.c_str(), (qsizetype)s.size());
}
#endif

SpellHighlighter::SpellHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    m_errorFmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    m_errorFmt.setUnderlineColor(Qt::red);
    loadDictionary();
}

SpellHighlighter::~SpellHighlighter()
{
#ifdef HAVE_HUNSPELL
    delete m_hunspell;
#endif
}

static bool extractResource(const QString &src, const QString &dst)
{
    if (QFileInfo::exists(dst))
        return true;
    QFile in(src);
    if (!in.open(QIODevice::ReadOnly))
        return false;
    QFile out(dst);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    while (!in.atEnd())
        out.write(in.read(65536));
    return true;
}

void SpellHighlighter::loadDictionary()
{
#ifdef HAVE_HUNSPELL
    // 1. Procurar no sistema
    static const QStringList sysDirs = {
        QStringLiteral("/usr/share/hunspell"),
        QStringLiteral("/usr/share/myspell/dicts"),
        QStringLiteral("/usr/local/share/hunspell"),
    };
    for (const QString &dir : sysDirs) {
        const QString aff = dir + QStringLiteral("/pt_BR.aff");
        const QString dic = dir + QStringLiteral("/pt_BR.dic");
        if (QFileInfo::exists(aff) && QFileInfo::exists(dic)) {
            m_hunspell = new Hunspell(aff.toUtf8().constData(),
                                      dic.toUtf8().constData());
            return;
        }
    }

    // 2. Extrair do resource embutido para cache
    const QString cacheDir =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/hunspell");
    QDir().mkpath(cacheDir);

    const QString cachedAff = cacheDir + QStringLiteral("/pt_BR.aff");
    const QString cachedDic = cacheDir + QStringLiteral("/pt_BR.dic");

    if (extractResource(QStringLiteral(":/dictionaries/pt_BR.aff"), cachedAff) &&
        extractResource(QStringLiteral(":/dictionaries/pt_BR.dic"), cachedDic)) {
        m_hunspell = new Hunspell(cachedAff.toUtf8().constData(),
                                  cachedDic.toUtf8().constData());
    }
#endif
}

bool SpellHighlighter::isLoaded() const
{
#ifdef HAVE_HUNSPELL
    return m_hunspell != nullptr;
#else
    return false;
#endif
}

bool SpellHighlighter::isCorrect(const QString &word) const
{
#ifdef HAVE_HUNSPELL
    if (!m_hunspell || word.length() < 3) return true;
    const QByteArray enc = cleanDictEncoding(m_hunspell);
    const QByteArray encoded = encodeWord(word, enc);
    return m_hunspell->spell(std::string(encoded.constData(), encoded.size()));
#else
    Q_UNUSED(word)
    return true;
#endif
}

QStringList SpellHighlighter::suggestions(const QString &word) const
{
#ifdef HAVE_HUNSPELL
    if (!m_hunspell) return {};
    const QByteArray enc = cleanDictEncoding(m_hunspell);
    const QByteArray encoded = encodeWord(word, enc);
    QStringList list;
    for (const std::string &s : m_hunspell->suggest(std::string(encoded.constData(), encoded.size())))
        list << decodeWord(s, enc);
    return list.mid(0, 8);
#else
    Q_UNUSED(word)
    return {};
#endif
}

void SpellHighlighter::highlightBlock(const QString &text)
{
#ifdef HAVE_HUNSPELL
    if (!m_hunspell) return;
    static const QRegularExpression wordRx(QStringLiteral("[\\p{L}]{3,}"));
    auto it = wordRx.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        if (!isCorrect(m.captured()))
            setFormat(m.capturedStart(), m.capturedLength(), m_errorFmt);
    }
#else
    Q_UNUSED(text)
#endif
}
