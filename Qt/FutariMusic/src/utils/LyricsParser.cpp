#include "LyricsParser.h"

#include <QRegularExpression>
#include <QTextBoundaryFinder>
#include <QVariantMap>
#include <algorithm>

namespace {
struct TimedLine {
    qint64 at = -1;
    QString source;
};

qint64 parseTime(const QRegularExpressionMatch& match) {
    const qint64 minutes = match.captured(1).toLongLong();
    const qint64 seconds = match.captured(2).toLongLong();
    const QString fraction = match.captured(3);
    const qint64 fractionMs = fraction.isEmpty()
                                  ? 0
                                  : fraction.left(3).leftJustified(3, QLatin1Char('0')).toLongLong();
    return (minutes * 60 + seconds) * 1000 + fractionMs;
}

QStringList graphemes(const QString& text) {
    QStringList result;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    finder.toStart();
    int start = 0;
    for (int end = finder.toNextBoundary(); end >= 0; end = finder.toNextBoundary()) {
        if (end > start) result.append(text.mid(start, end - start));
        start = end;
    }
    return result;
}

void appendTimedCharacters(QVariantList& result, const QString& text, qint64 start, qint64 end) {
    const QStringList units = graphemes(text);
    if (units.isEmpty()) return;
    // Some producers timestamp syllables rather than every grapheme; divide only that interval.
    const qint64 span = qMax<qint64>(units.size(), end - start);
    for (qsizetype index = 0; index < units.size(); ++index) {
        const qint64 from = start + span * index / units.size();
        const qint64 to = start + span * (index + 1) / units.size();
        result.append(QVariantMap{{"text", units.at(index)}, {"startMs", from}, {"endMs", to}});
    }
}

QVariantList parseCharacters(const QString& source, qint64 lineStart, qint64 nextLineStart,
                             qint64 offset) {
    static const QRegularExpression marker(R"(<(\d+):(\d{1,2})(?:\.(\d+))?>)");
    auto matches = marker.globalMatch(source);
    QList<QRegularExpressionMatch> markers;
    while (matches.hasNext()) markers.append(matches.next());

    QVariantList result;
    if (markers.isEmpty()) return result;
    if (markers.first().capturedStart() > 0)
        appendTimedCharacters(result, source.left(markers.first().capturedStart()), lineStart,
                              qMax<qint64>(0, parseTime(markers.first()) + offset));

    for (qsizetype index = 0; index < markers.size(); ++index) {
        const auto& current = markers.at(index);
        const qint64 start = qMax<qint64>(0, parseTime(current) + offset);
        const int contentStart = current.capturedEnd();
        const int contentEnd = index + 1 < markers.size()
                                   ? markers.at(index + 1).capturedStart()
                                   : source.size();
        qint64 end = index + 1 < markers.size()
                         ? qMax<qint64>(0, parseTime(markers.at(index + 1)) + offset)
                         : (nextLineStart > start ? nextLineStart : start + 800);
        // A stanza gap should not keep the last syllable highlighted for several seconds.
        end = qMin(end, start + 6000);
        appendTimedCharacters(result, source.mid(contentStart, contentEnd - contentStart), start,
                              qMax(start + 1, end));
    }
    return result;
}

bool isMetadata(const QString& line) {
    static const QRegularExpression metadata(
        R"(^\s*\[(?:ti|ar|al|by|re|ve|offset|kana|la|length):.*\]\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    return metadata.match(line).hasMatch();
}

bool isLikelyCredit(const QString& text) {
    static const QRegularExpression credits(
        R"(^(?:作词|作曲|词曲|词|曲|作詞|詞曲|詞|編曲|編詞|编曲|制作人|制作|监制|演唱|配唱|录音|混音|母带|吉他|贝斯|鼓|钢琴|弦乐|乐器|录音室|录音工程|配乐|音乐制作|Lyrics?|Music|Composer|Arranged|Arrangement|Producer|Produced|Vocal|Guitar|Bass|Drums|Piano)\s*[:：])",
        QRegularExpression::CaseInsensitiveOption);
    // This is a review hint only; credit lines stay in the stored and displayed lyrics.
    return credits.match(text.trimmed()).hasMatch();
}
}  // namespace

bool LyricsParser::hasKaraokeTimestamps(const QString& source) {
    static const QRegularExpression marker(R"(<\d+:\d{1,2}(?:\.\d+)?>)");
    return marker.match(source).hasMatch();
}

int LyricsParser::countLikelyCreditLines(const QString& source) {
    static const QRegularExpression outerTags(R"(^\s*(?:\[\d+:\d{1,2}(?:\.\d+)?\])+)");
    static const QRegularExpression inlineTime(R"(<\d+:\d{1,2}(?:\.\d+)?>)");
    int count = 0;
    for (QString line : source.split(QRegularExpression(QStringLiteral("\\r?\\n")))) {
        line.remove(outerTags);
        line.remove(inlineTime);
        if (isLikelyCredit(line)) ++count;
    }
    return count;
}

QVariantList LyricsParser::parse(const QString& source) {
    static const QRegularExpression outerTags(R"(^\s*((?:\[\d+:\d{1,2}(?:\.\d+)?\])+)(.*)$)");
    static const QRegularExpression outerTime(R"(\[(\d+):(\d{1,2})(?:\.(\d+))?\])");
    static const QRegularExpression inlineTime(R"(<\d+:\d{1,2}(?:\.\d+)?>)");
    static const QRegularExpression offsetTag(R"(\[offset:([+-]?\d+)\])",
                                              QRegularExpression::CaseInsensitiveOption);
    qint64 offset = 0;
    const auto offsetMatch = offsetTag.match(source);
    if (offsetMatch.hasMatch()) offset = offsetMatch.captured(1).toLongLong();

    QList<TimedLine> lines;
    for (const QString& line : source.split(QRegularExpression(QStringLiteral("\\r?\\n")))) {
        if (line.trimmed().isEmpty() || isMetadata(line)) continue;
        const auto tagged = outerTags.match(line);
        if (!tagged.hasMatch()) {
            lines.append({-1, line.trimmed()});
            continue;
        }
        const QString body = tagged.captured(2);
        auto timeMatches = outerTime.globalMatch(tagged.captured(1));
        while (timeMatches.hasNext()) {
            const qint64 at = qMax<qint64>(0, parseTime(timeMatches.next()) + offset);
            lines.append({at, body});
        }
    }

    QVariantList result;
    for (qsizetype index = 0; index < lines.size(); ++index) {
        const TimedLine& line = lines.at(index);
        QString text = line.source;
        text.remove(inlineTime);
        text = text.trimmed();
        if (text.isEmpty()) continue;

        qint64 nextLineStart = -1;
        for (qsizetype next = index + 1; next < lines.size(); ++next) {
            if (lines.at(next).at >= 0) {
                nextLineStart = lines.at(next).at;
                break;
            }
        }
        const bool karaoke = hasKaraokeTimestamps(line.source);
        result.append(QVariantMap{{"at", line.at},
                                  {"text", text},
                                  {"karaoke", karaoke},
                                  {"credit", isLikelyCredit(text)},
                                  {"characters", karaoke
                                                       ? parseCharacters(line.source, line.at,
                                                                         nextLineStart, offset)
                                                       : QVariantList{}}});
    }
    return result;
}
