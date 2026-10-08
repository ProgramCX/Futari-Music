#include "AudioMetadataReader.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QMediaMetaData>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <utility>

namespace {
constexpr qint64 kMaxFlacMetadataBlockBytes = 32LL * 1024 * 1024;

QString metadataText(const QVariant &value) {
    if (value.metaType().id() == QMetaType::QStringList) return value.toStringList().join(QStringLiteral(", ")).trimmed();
    if (value.canConvert<QString>()) return value.toString().trimmed();
    return {};
}

bool readUint32LittleEndian(const QByteArray &bytes, qsizetype &offset, quint32 &value) {
    if (offset < 0 || bytes.size() - offset < 4) return false;
    const auto *data = reinterpret_cast<const uchar *>(bytes.constData() + offset);
    value = static_cast<quint32>(data[0]) | (static_cast<quint32>(data[1]) << 8) |
            (static_cast<quint32>(data[2]) << 16) | (static_cast<quint32>(data[3]) << 24);
    offset += 4;
    return true;
}

bool readUint32BigEndian(const QByteArray &bytes, qsizetype &offset, quint32 &value) {
    if (offset < 0 || bytes.size() - offset < 4) return false;
    const auto *data = reinterpret_cast<const uchar *>(bytes.constData() + offset);
    value = (static_cast<quint32>(data[0]) << 24) | (static_cast<quint32>(data[1]) << 16) |
            (static_cast<quint32>(data[2]) << 8) | static_cast<quint32>(data[3]);
    offset += 4;
    return true;
}

struct FlacMetadata {
    QHash<QString, QString> text;
    QImage cover;
    QByteArray coverData;
    QString coverExtension;
    quint32 coverType = 0;
    qint64 durationMs = 0;
};

void parseVorbisComments(const QByteArray &block, FlacMetadata &metadata) {
    qsizetype offset = 0;
    quint32 vendorLength = 0;
    if (!readUint32LittleEndian(block, offset, vendorLength) || vendorLength > static_cast<quint32>(block.size() - offset)) return;
    offset += vendorLength;

    quint32 commentCount = 0;
    if (!readUint32LittleEndian(block, offset, commentCount) || commentCount > 4096) return;
    for (quint32 i = 0; i < commentCount; ++i) {
        quint32 commentLength = 0;
        if (!readUint32LittleEndian(block, offset, commentLength) || commentLength > static_cast<quint32>(block.size() - offset)) return;
        const QByteArray comment = block.mid(offset, static_cast<qsizetype>(commentLength));
        offset += commentLength;
        const qsizetype separator = comment.indexOf('=');
        if (separator <= 0) continue;

        QString key = QString::fromLatin1(comment.first(separator)).trimmed().toUpper();
        key.remove(QLatin1Char(' '));
        key.remove(QLatin1Char('_'));
        const QString value = QString::fromUtf8(comment.sliced(separator + 1)).trimmed();
        QString canonicalKey;
        if (key == QStringLiteral("TITLE")) canonicalKey = QStringLiteral("title");
        else if (key == QStringLiteral("ARTIST")) canonicalKey = QStringLiteral("artist");
        else if (key == QStringLiteral("ALBUM")) canonicalKey = QStringLiteral("album");
        else if (key == QStringLiteral("ALBUMARTIST")) canonicalKey = QStringLiteral("albumArtist");
        if (canonicalKey.isEmpty() || value.isEmpty()) continue;

        QString &existing = metadata.text[canonicalKey];
        if (!existing.isEmpty()) existing.append(QStringLiteral(", "));
        existing.append(value);
    }
}

bool parseFlacPicture(const QByteArray &block, quint32 &pictureType, QImage &image,
                      QByteArray &imageData, QString &imageExtension) {
    qsizetype offset = 0;
    quint32 mimeLength = 0;
    quint32 descriptionLength = 0;
    quint32 imageLength = 0;
    if (!readUint32BigEndian(block, offset, pictureType) ||
        !readUint32BigEndian(block, offset, mimeLength) || mimeLength > static_cast<quint32>(block.size() - offset)) return false;
    const QByteArray mime = block.mid(offset, static_cast<qsizetype>(mimeLength)).toLower();
    offset += mimeLength;
    if (!readUint32BigEndian(block, offset, descriptionLength) || descriptionLength > static_cast<quint32>(block.size() - offset)) return false;
    offset += descriptionLength;

    // Width, height, color depth and indexed-color count.
    if (block.size() - offset < 16) return false;
    offset += 16;
    if (!readUint32BigEndian(block, offset, imageLength) || imageLength == 0 ||
        imageLength > static_cast<quint32>(block.size() - offset)) return false;
    QByteArray encoded = block.mid(offset, static_cast<qsizetype>(imageLength));
    image = QImage::fromData(encoded);
    if (image.isNull()) return false;
    if (mime == QByteArrayLiteral("image/jpeg") || mime == QByteArrayLiteral("image/jpg")) imageExtension = QStringLiteral("jpg");
    else if (mime == QByteArrayLiteral("image/png")) imageExtension = QStringLiteral("png");
    else if (mime == QByteArrayLiteral("image/webp")) imageExtension = QStringLiteral("webp");
    else imageExtension.clear();
    if (!imageExtension.isEmpty()) imageData = std::move(encoded);
    return true;
}

FlacMetadata readFlacMetadata(const QString &path) {
    FlacMetadata metadata;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.read(4) != QByteArrayLiteral("fLaC")) return metadata;

    while (file.pos() + 4 <= file.size()) {
        const QByteArray header = file.read(4);
        if (header.size() != 4) break;
        const auto *headerBytes = reinterpret_cast<const uchar *>(header.constData());
        const bool lastBlock = (headerBytes[0] & 0x80) != 0;
        const quint8 blockType = headerBytes[0] & 0x7f;
        const quint32 blockLength = (static_cast<quint32>(headerBytes[1]) << 16) |
                                    (static_cast<quint32>(headerBytes[2]) << 8) | headerBytes[3];
        if (blockLength > static_cast<quint64>(file.size() - file.pos())) break;

        if ((blockType == 0 || blockType == 4 || blockType == 6) && blockLength <= kMaxFlacMetadataBlockBytes) {
            const QByteArray block = file.read(static_cast<qint64>(blockLength));
            if (block.size() != static_cast<qsizetype>(blockLength)) break;
            if (blockType == 0 && block.size() >= 18) {
                quint64 packed = 0;
                for (qsizetype i = 10; i < 18; ++i)
                    packed = (packed << 8) | static_cast<uchar>(block.at(i));
                const quint32 sampleRate = static_cast<quint32>((packed >> 44) & 0xfffff);
                const quint64 totalSamples = packed & 0xfffffffffULL;
                if (sampleRate > 0 && totalSamples > 0)
                    metadata.durationMs = static_cast<qint64>((totalSamples * 1000ULL) / sampleRate);
            } else if (blockType == 4) {
                parseVorbisComments(block, metadata);
            } else if (blockType == 6) {
                quint32 pictureType = 0;
                QImage picture;
                QByteArray pictureData;
                QString pictureExtension;
                if (parseFlacPicture(block, pictureType, picture, pictureData, pictureExtension) &&
                    (metadata.cover.isNull() || (pictureType == 3 && metadata.coverType != 3))) {
                    metadata.cover = std::move(picture);
                    metadata.coverData = std::move(pictureData);
                    metadata.coverExtension = std::move(pictureExtension);
                    metadata.coverType = pictureType;
                }
            }
        } else if (!file.seek(file.pos() + blockLength)) {
            break;
        }
        if (lastBlock) break;
    }
    return metadata;
}
}

AudioMetadataReader::AudioMetadataReader(QObject *parent) : QObject(parent) {
    m_settleTimer.setSingleShot(true);
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(12000);
    connect(&m_settleTimer, &QTimer::timeout, this, [this] { finish(); });
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this] { finish(); });
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            m_settleTimer.start(180);
        } else if (status == QMediaPlayer::InvalidMedia) {
            finish();
        }
    });
    connect(&m_player, &QMediaPlayer::metaDataChanged, this, [this] {
        if (m_player.mediaStatus() == QMediaPlayer::LoadedMedia || m_player.mediaStatus() == QMediaPlayer::BufferedMedia)
            m_settleTimer.start(120);
    });
}

void AudioMetadataReader::inspect(const QString &audioUrl, const QString &requestId) {
    m_settleTimer.stop();
    m_timeoutTimer.stop();
    m_player.stop();
    m_audioUrl = QUrl(audioUrl).toLocalFile();
    m_requestId = requestId;
    if (m_audioUrl.isEmpty() || !QFileInfo(m_audioUrl).isFile()) {
        finish();
        return;
    }
    m_player.setSource(QUrl::fromLocalFile(m_audioUrl));
    m_timeoutTimer.start();
}

void AudioMetadataReader::finish() {
    if (m_requestId.isEmpty()) return;
    m_settleTimer.stop();
    m_timeoutTimer.stop();
    const QString requestId = std::exchange(m_requestId, {});
    const QFileInfo audioInfo(m_audioUrl);
    const QMediaMetaData tags = m_player.metaData();
    const FlacMetadata flac = audioInfo.suffix().compare(QStringLiteral("flac"), Qt::CaseInsensitive) == 0
            ? readFlacMetadata(m_audioUrl) : FlacMetadata{};
    const auto textValue = [&flac, &tags](const QString &key, QMediaMetaData::Key nativeKey) {
        const QString fromFile = flac.text.value(key);
        return fromFile.isEmpty() ? metadataText(tags.value(nativeKey)) : fromFile;
    };
    const QString title = textValue(QStringLiteral("title"), QMediaMetaData::Title);
    const QString artist = textValue(QStringLiteral("artist"), QMediaMetaData::ContributingArtist);
    const QString album = textValue(QStringLiteral("album"), QMediaMetaData::AlbumTitle);
    const QString albumArtist = textValue(QStringLiteral("albumArtist"), QMediaMetaData::AlbumArtist);
    qint64 durationMs = m_player.duration();
    if (durationMs <= 0) durationMs = flac.durationMs;
    QVariantMap metadata{
        {QStringLiteral("title"), title},
        {QStringLiteral("artist"), artist},
        {QStringLiteral("album"), album},
        {QStringLiteral("albumArtist"), albumArtist},
        {QStringLiteral("durationMs"), durationMs},
        {QStringLiteral("format"), audioInfo.suffix().toLower()},
        {QStringLiteral("fileSize"), audioInfo.size()}
    };
    if (metadata.value(QStringLiteral("artist")).toString().isEmpty())
        metadata.insert(QStringLiteral("artist"), metadataText(tags.value(QMediaMetaData::Author)));

    QByteArray artworkData;
    QString artworkExtension = QStringLiteral("png");
    QImage artwork = flac.cover;
    if (!flac.cover.isNull()) {
        artworkData = flac.coverData;
        if (!flac.coverExtension.isEmpty()) artworkExtension = flac.coverExtension;
    } else {
        artwork = tags.value(QMediaMetaData::CoverArtImage).value<QImage>();
        if (artwork.isNull()) artwork = tags.value(QMediaMetaData::ThumbnailImage).value<QImage>();
    }
    metadata.insert(QStringLiteral("available"), !title.isEmpty() || !artist.isEmpty() || !album.isEmpty() ||
                    !albumArtist.isEmpty() || !artwork.isNull());
    if (!artwork.isNull()) {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/artwork");
        if (QDir().mkpath(directory)) {
            const QString path = directory + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::Id128) + QLatin1Char('.') + artworkExtension;
            QSaveFile file(path);
            const bool written = file.open(QIODevice::WriteOnly) &&
                    (artworkData.isEmpty() ? artwork.save(&file, "PNG") : file.write(artworkData) == artworkData.size());
            if (written && file.commit())
                metadata.insert(QStringLiteral("coverUrl"), QUrl::fromLocalFile(path).toString());
        }
    }
    m_player.stop();
    m_player.setSource({});
    emit metadataReady(requestId, metadata);
}
