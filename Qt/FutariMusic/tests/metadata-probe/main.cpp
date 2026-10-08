#include "../../src/AudioMetadataReader.h"

#include <QBuffer>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <functional>
#include <memory>

// 使用实际解析类和实际上传页，覆盖元数据事件到封面预览的完整链路。
class TestController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool darkMode READ darkMode CONSTANT)
    Q_PROPERTY(QVariantList albums READ albums CONSTANT)
    Q_PROPERTY(QVariantList albumMatches READ albums NOTIFY albumMatchesChanged)
public:
    bool darkMode() const { return false; }
    QVariantList albums() const { return {}; }
    Q_INVOKABLE void refreshAlbums() {}
    Q_INVOKABLE void searchAlbumMatches(const QString &, const QString &) { emit albumMatchesChanged(); }
signals:
    void audioMetadataReady(const QString &requestId, const QVariantMap &metadata);
    void albumMatchesChanged();
};

namespace {
void appendUint32(QByteArray &bytes, quint32 value, bool littleEndian = false) {
    for (int i = 0; i < 4; ++i) {
        const int shift = littleEndian ? i * 8 : (3 - i) * 8;
        bytes.append(static_cast<char>((value >> shift) & 0xff));
    }
}
void appendBlock(QByteArray &bytes, quint8 type, const QByteArray &block, bool last = false) {
    bytes.append(static_cast<char>(type | (last ? 0x80 : 0)));
    for (int shift : {16, 8, 0}) bytes.append(static_cast<char>((block.size() >> shift) & 0xff));
    bytes.append(block);
}
QString createFixture(const QString &directory) {
    QImage image(16, 16, QImage::Format_RGB32);
    image.fill(QColor("#13ba78"));
    QByteArray pictureBytes;
    QBuffer buffer(&pictureBytes);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG")) return {};

    QByteArray picture;
    appendUint32(picture, 0); // 反馈文件的图片类型为 Other，不能仅接受 Front Cover。
    const QByteArray mime("image/png");
    appendUint32(picture, mime.size()); picture.append(mime);
    appendUint32(picture, 0);
    appendUint32(picture, 16); appendUint32(picture, 16);
    appendUint32(picture, 24); appendUint32(picture, 0);
    appendUint32(picture, pictureBytes.size()); picture.append(pictureBytes);

    QByteArray comments;
    appendUint32(comments, 0, true);
    const QList<QByteArray> tags{"TITLE=Test title", "ARTIST=Test singer", "ALBUM=Test album"};
    appendUint32(comments, tags.size(), true);
    for (const auto &tag : tags) { appendUint32(comments, tag.size(), true); comments.append(tag); }
    QByteArray flac("fLaC");
    appendBlock(flac, 0, QByteArray(34, '\0'));
    appendBlock(flac, 4, comments);
    appendBlock(flac, 6, picture, true);
    const QString path = directory + "/metadata.flac";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(flac) != flac.size()) return {};
    return path;
}
bool waitUntil(const std::function<bool()> &condition, int milliseconds = 15000) {
    if (condition()) return true;
    QEventLoop loop;
    QTimer poll;
    poll.setInterval(20);
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] { if (condition()) loop.quit(); });
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    poll.start();
    loop.exec();
    return condition();
}
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    QTemporaryDir fixture;
    const QString audioPath = app.arguments().size() > 1 ? app.arguments().at(1) : createFixture(fixture.path());
    QTextStream output(stdout);
    auto check = [&output](bool ok, const char *message) {
        output << (ok ? "PASS " : "FAIL ") << message << Qt::endl;
        return ok;
    };
    if (!check(!audioPath.isEmpty(), "audio fixture")) return 1;

    TestController controller;
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/imports");
    engine.rootContext()->setContextProperty("appController", &controller);
    int scriptErrors = 0;
    QObject::connect(&engine, &QQmlEngine::warnings, &app, [&](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) {
            if (warning.description().contains("ReferenceError") || warning.description().contains("TypeError")) {
                ++scriptErrors;
                output << warning.toString() << Qt::endl;
            }
        }
    });
    QQuickWindow window;
    window.resize(1000, 1000);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(FUTARI_SOURCE_DIR "/qml/pages/SingleUploadDialog.qml")));
    std::unique_ptr<QObject> dialog(component.createWithInitialProperties({
        {"parent", QVariant::fromValue(window.contentItem())}
    }));
    if (!dialog) { output << component.errorString() << Qt::endl; return 1; }
    bool ok = check(QMetaObject::invokeMethod(dialog.get(), "resetForm") && scriptErrors == 0, "dialog reset without stale field references");
    dialog->setProperty("audioUrl", QUrl::fromLocalFile(audioPath).toString());
    dialog->setProperty("pendingRequestId", "regression");
    QVariantMap parsed;
    AudioMetadataReader reader;
    bool received = false;
    QObject::connect(&reader, &AudioMetadataReader::metadataReady, &controller, [&](const QString &id, const QVariantMap &metadata) {
        parsed = metadata;
        emit controller.audioMetadataReady(id, metadata);
        received = true;
    });
    reader.inspect(QUrl::fromLocalFile(audioPath).toString(), "regression");
    ok &= check(waitUntil([&] { return received; }), "actual reader emits metadata");
    const QString cover = parsed.value("coverUrl").toString();
    ok &= check(!cover.isEmpty() && !QImage(QUrl(cover).toLocalFile()).isNull(), "actual reader extracts readable cover");
    ok &= check(dialog->property("embeddedCoverUrl").toString() == cover && !cover.isEmpty(), "upload dialog receives embedded cover");
    QObject *preview = dialog->findChild<QObject *>("embeddedCoverPreview");
    ok &= check(preview && waitUntil([&] { return preview->property("status").toInt() == 1; }, 3000), "QML image preview is ready");
    QObject *status = dialog->findChild<QObject *>("metadataStatus");
    ok &= check(status && status->property("text").toString() == QStringLiteral("已读取音频标签"), "metadata handler reaches completion");
    QObject *toggle = dialog->findChild<QObject *>("toggleNewAlbum");
    QObject *albumArtist = dialog->findChild<QObject *>("newAlbumArtist");
    if (!check(toggle && albumArtist, "album controls available")) return 1;
    QMetaObject::invokeMethod(toggle, "clicked");
    const QString expectedArtist = parsed.value("albumArtist").toString().isEmpty() ? parsed.value("artist").toString() : parsed.value("albumArtist").toString();
    ok &= check(albumArtist->property("text").toString() == expectedArtist && !expectedArtist.isEmpty(), "new album defaults to tagged artist or current singer");
    if (parsed.value("albumArtist").toString().isEmpty()) {
        QMetaObject::invokeMethod(toggle, "clicked");
        QObject *singer = dialog->findChild<QObject *>("songArtist");
        if (!check(singer != nullptr, "singer field available")) return 1;
        singer->setProperty("text", "Edited singer");
        QMetaObject::invokeMethod(toggle, "clicked");
        ok &= check(albumArtist->property("text").toString() == "Edited singer", "default artist uses current edited singer");
    }
    ok &= check(scriptErrors == 0, "no QML script errors in metadata flow");
    QFile::remove(QUrl(cover).toLocalFile());
    return ok ? 0 : 1;
}

#include "main.moc"
