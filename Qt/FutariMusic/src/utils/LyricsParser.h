#ifndef FUTARI_LYRICS_PARSER_H
#define FUTARI_LYRICS_PARSER_H

#include <QVariantList>
#include <QString>

class LyricsParser final {
public:
    static QVariantList parse(const QString& source);
    static bool hasKaraokeTimestamps(const QString& source);
    static int countLikelyCreditLines(const QString& source);
};

#endif  // FUTARI_LYRICS_PARSER_H
