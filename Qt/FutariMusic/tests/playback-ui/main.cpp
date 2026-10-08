#include "../../src/AppController.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QHttpMultiPart>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <functional>

namespace {
bool waitUntil(const std::function<bool()> &condition, int timeout = 5000) {
    if (condition()) return true;
    QEventLoop loop;
    QTimer poll; poll.setInterval(20);
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] { if (condition()) loop.quit(); });
    QTimer::singleShot(timeout, &loop, &QEventLoop::quit); poll.start(); loop.exec();
    return condition();
}
QList<QQuickItem *> visualItems(QQuickItem *root) {
    QList<QQuickItem *> items;
    for (auto *child : root->childItems()) { items.append(child); items.append(visualItems(child)); }
    return items;
}
QByteArray silentWave() {
    QByteArray bytes;
    auto append = [&](quint32 value, int size) { for (int i = 0; i < size; ++i) bytes.append(char((value >> (i * 8)) & 0xff)); };
    const int size = 8000 * 2 * 10;
    bytes += "RIFF"; append(size + 36, 4); bytes += "WAVEfmt "; append(16, 4); append(1, 2); append(1, 2);
    append(8000, 4); append(16000, 4); append(2, 2); append(16, 2); bytes += "data"; append(size, 4); bytes += QByteArray(size, '\0');
    return bytes;
}
// 替身服务只运行在随机本机端口；客户端仍走实际 HTTP/WS 和鉴权代码路径。
class Backend : public QObject {
public:
    QTcpServer http;
    QWebSocketServer ws{"Regression", QWebSocketServer::NonSecureMode};
    QWebSocket *peer = nullptr;
    QJsonArray playlists;
    QJsonArray serverPlaylists;
    QJsonArray queue;
    QJsonArray uploaded;
    QList<QByteArray> uploadBodies;
    QList<QByteArray> multipartTypes;
    bool delayLyrics = false;
    int lyricRequests = 0;
    bool failNextUpload = false;
    int playlistRequests = 0;
    bool control = true;
    bool activeRoom = false;
    bool roomExists = true;
    int roomDeleteRequests = 0;
    qint64 current = 0;
    const QByteArray audio = silentWave();
    Backend() {
        http.listen(QHostAddress::LocalHost, 0);
        connect(&http, &QTcpServer::newConnection, this, [this] {
            auto *socket = http.nextPendingConnection();
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                const QByteArray bytes = socket->peek(socket->bytesAvailable());
                const int end = bytes.indexOf("\r\n\r\n");
                if (end < 0) return;
                if (bytes.first(end).toLower().contains("upgrade: websocket")) {
                    socket->disconnect(this); ws.handleConnection(socket); return;
                }
                int length = 0;
                for (const auto &line : bytes.first(end).split('\n')) if (line.toLower().startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
                if (bytes.size() < end + 4 + length) return;
                socket->readAll();
                const auto first = bytes.first(bytes.indexOf('\n')).trimmed().split(' ');
                const QString path = QString::fromUtf8(first.value(1)).section('?', 0, 0);
                const QJsonObject body = QJsonDocument::fromJson(bytes.mid(end + 4, length)).object();
                if (path == "/api/songs/upload" || path == "/api/test-edit") {
                    for (const auto &line : bytes.first(end).split('\n'))
                        if (line.toLower().startsWith("content-type:")) multipartTypes.append(line.mid(13).trimmed());
                    uploadBodies.append(bytes.mid(end + 4, length));
                    if (failNextUpload) {
                        failNextUpload = false;
                        respond(socket, QJsonDocument(QJsonObject{{"code", 3003}, {"message", "test upload failure"}}).toJson(), "application/json");
                        return;
                    }
                    uploaded.append(song(100 + uploaded.size()));
                    respond(socket, QJsonDocument(QJsonObject{{"code", 0}, {"data", uploaded.last()}}).toJson(), "application/json");
                    return;
                }
                if (path.endsWith("/lyrics")) {
                    const qint64 songId = path.section('/', 3, 3).toLongLong();
                    ++lyricRequests;
                    const QByteArray response = QJsonDocument(QJsonObject{{"code", 0}, {"data", QJsonObject{{"songId", songId}, {"lyrics", QString("[00:01.00]Lyrics %1").arg(songId)}}}}).toJson();
                    if (delayLyrics && songId == 1)
                        QTimer::singleShot(350, socket, [this, socket, response] { respond(socket, response, "application/json"); });
                    else respond(socket, response, "application/json");
                    return;
                }
                if (path.endsWith("/file")) { respond(socket, audio, "audio/wav"); return; }
                const QJsonValue data = route(first.value(0), path, body);
                respond(socket, QJsonDocument(QJsonObject{{"code", 0}, {"message", "ok"}, {"data", data}}).toJson(QJsonDocument::Compact), "application/json");
            });
        });
        connect(&ws, &QWebSocketServer::newConnection, this, [this] {
            peer = ws.nextPendingConnection();
            connect(peer, &QWebSocket::textMessageReceived, this, [this](const QString &text) {
                const auto message = QJsonDocument::fromJson(text.toUtf8()).object();
                const QString type = message.value("type").toString();
                const auto data = message.value("data").toObject();
                if (type == "PLAYLIST_UPDATE") { queue = data.value("songIds").toArray(); ++playlistRequests; broadcast(type, queue); }
                if (type == "PLAY") { current = data.value("songId").toInteger(); broadcast("PLAY", playback()); }
            });
        });
    }
    QJsonObject song(qint64 id) const { return {{"id", id}, {"title", QString("Song %1").arg(id)}, {"artist", "Singer"}, {"album", "Album"}, {"coverUrl", QJsonValue(QJsonValue::Null)}, {"format", "wav"}, {"hash", QString::fromLatin1(QCryptographicHash::hash(audio, QCryptographicHash::Sha256).toHex())}, {"fileSize", audio.size()}, {"durationMs", 10000}}; }
    QJsonObject playback() const { return {{"currentSongId", current ? QJsonValue(current) : QJsonValue()}, {"status", "paused"}, {"positionMs", 0}, {"serverTimestamp", QDateTime::currentMSecsSinceEpoch()}}; }
    QJsonObject state() const { return {{"room", QJsonObject{{"id", 7}, {"name", "Room"}, {"ownerId", control ? 1 : 2}}}, {"songIds", queue}, {"controllerIds", QJsonArray{}}, {"members", QJsonArray{}}, {"playback", playback()}}; }
    void broadcast(const QString &type, const QJsonValue &data) { if (peer) peer->sendTextMessage(QString::fromUtf8(QJsonDocument(QJsonObject{{"type", type}, {"data", data}}).toJson(QJsonDocument::Compact))); }
    QJsonValue route(const QByteArray &method, const QString &path, const QJsonObject &body) {
        if (path == "/api/auth/login") return QJsonObject{{"userId", 1}, {"token", "test-session"}, {"role", "ADMIN"}, {"canUpload", true}};
        if (path == "/api/songs") return QJsonObject{{"total", 3}, {"list", QJsonArray{song(1), song(2), song(3)}}};
        if (path == "/api/rooms/current") return activeRoom ? QJsonValue(state()) : QJsonValue(QJsonObject{});
        if (path == "/api/rooms" && method == "GET") return QJsonObject{{"total", roomExists ? 1 : 0}, {"list", roomExists ? QJsonArray{QJsonObject{{"id", 7}, {"name", "Room"}, {"ownerId", control ? 1 : 2}}} : QJsonArray{}}};
        if (path == "/api/rooms/7" && method == "DELETE") {
            ++roomDeleteRequests; activeRoom = false; roomExists = false;
            broadcast("ROOM_DELETED", QJsonObject{{"roomId", 7}}); return {};
        }
        if (path.endsWith("/state")) return state();
        if (path == "/api/rooms" && method == "POST") { activeRoom = true; roomExists = true; return QJsonObject{{"id", 7}}; }
        if (path.endsWith("/leave")) { activeRoom = false; return {}; }
        if (path == "/api/playlists" || path == "/api/server-playlists") {
            auto &lists = path.contains("server-playlists") ? serverPlaylists : playlists;
            if (method == "POST") { QJsonObject list{{"id", lists.size() + 10}, {"name", body.value("name")}, {"songs", QJsonArray{}}}; lists.append(list); return list; }
            return lists;
        }
        if ((path.startsWith("/api/playlists/") || path.startsWith("/api/server-playlists/")) && (path.contains("/songs") || path.endsWith("/order"))) {
            auto &lists = path.contains("server-playlists") ? serverPlaylists : playlists;
            const qint64 id = path.section('/', 3, 3).toLongLong();
            for (int i = 0; i < lists.size(); ++i) {
                auto list = lists[i].toObject(); if (list.value("id").toInteger() != id) continue;
                auto songs = list.value("songs").toArray();
                if (path.endsWith("/order")) { songs = {}; for (const auto &songId : body.value("songIds").toArray()) songs.append(song(songId.toInteger())); }
                else if (method == "DELETE") { const qint64 removed = path.section('/', 5, 5).toLongLong(); for (int j = songs.size() - 1; j >= 0; --j) if (songs[j].toObject().value("id").toInteger() == removed) songs.removeAt(j); }
                else for (const auto &songId : body.value("songIds").toArray()) songs.append(song(songId.toInteger()));
                list["songs"] = songs; lists[i] = list;
            }
            return QJsonObject{{"added", 1}, {"duplicated", 0}};
        }
        if (path.startsWith("/api/playlists/") || path.startsWith("/api/server-playlists/")) {
            auto &lists = path.contains("server-playlists") ? serverPlaylists : playlists;
            const qint64 id = path.section('/', 3, 3).toLongLong();
            for (int i = 0; i < lists.size(); ++i) if (lists[i].toObject().value("id").toInteger() == id) {
                if (method == "DELETE") lists.removeAt(i);
                else if (method == "PUT") { auto list = lists[i].toObject(); list["name"] = body.value("name"); lists[i] = list; }
                break;
            }
            return {};
        }
        if (path == "/api/songs/mine") return QJsonObject{{"total", uploaded.size()}, {"list", uploaded}};
        if (path.startsWith("/api/songs/") && !path.endsWith("/lyrics")) return song(path.section('/', 3, 3).toLongLong());
        return QJsonArray{};
    }
    void respond(QTcpSocket *socket, const QByteArray &bytes, const QByteArray &type) {
        socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: " + type + "\r\nContent-Length: " + QByteArray::number(bytes.size()) + "\r\n\r\n" + bytes); socket->disconnectFromHost();
    }
};
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QQuickStyle::setStyle("Basic"); QQuickWindow::setDefaultAlphaBuffer(true);
    QTemporaryDir directory;
    QCoreApplication::setApplicationName("FutariUiRegression-" + QFileInfo(directory.path()).fileName());
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    Backend server;
    AppController controller;
    controller.setServerUrl(QString("http://127.0.0.1:%1/").arg(server.http.serverPort()));
    controller.setCacheDirectory(directory.path() + "/cache"); controller.player()->setVolume(0);
    QTextStream output(stdout); bool ok = true;
    auto check = [&](bool result, const char *name) { output << (result ? "PASS " : "FAIL ") << name << Qt::endl; ok &= result; };
    QSettings isolatedSettings(QSettings::defaultFormat(), QSettings::UserScope, "Futari", "FutariMusic");
    check(isolatedSettings.fileName().startsWith(directory.path()), "settings use temporary INI storage instead of Windows registry");
    controller.login("test", "test-password");
    check(waitUntil([&] { return controller.authenticated() && server.peer && controller.songs().size() == 3; }), "real client HTTP login and WS connection");
    controller.addToLocalQueue(1); controller.addToLocalQueue(2); controller.addToLocalQueue(3);
    controller.playSong(server.song(1).toVariantMap());
    check(waitUntil([&] { return !controller.player()->loading() && controller.player()->duration() > 0; }), "real audio resource downloaded and loaded");
    controller.player()->pause(); controller.seek(1000);
    check(waitUntil([&] { return controller.player()->position() >= 1000; }), "local playback position available");
    controller.playNext(server.song(3).toVariantMap());
    check(controller.localQueue() == QVariantList{1LL, 3LL, 2LL}, "play next moves existing track without duplicates");
    const QVariantList localBefore = controller.localQueue();
    controller.createRoom("Room");
    check(waitUntil([&] { return !controller.roomState().isEmpty(); }), "join room with separate shared queue");
    controller.addToQueue(2);
    check(waitUntil([&] { return server.queue == QJsonArray{2} && controller.roomState().value("songIds").toList() == QVariantList{2LL}; }), "default add targets room via WebSocket");
    controller.playSong(server.song(2).toVariantMap());
    check(waitUntil([&] { return server.current == 2 && controller.roomState().value("playback").toMap().value("currentSongId").toLongLong() == 2; }), "play targets current room");
    controller.playNext(server.song(3).toVariantMap());
    check(waitUntil([&] { return server.queue == QJsonArray{2, 3}; }), "room next track order broadcast");
    check(controller.localQueue() == localBefore, "room operations preserve personal queue");
    server.control = false; controller.refreshRoomState();
    check(waitUntil([&] { return !controller.canControl(); }), "server controls permission");
    const int updates = server.playlistRequests;
    controller.addToRoomQueue(1); controller.playNext(server.song(1).toVariantMap());
    QTest::qWait(100);
    check(server.playlistRequests == updates, "unauthorized user cannot edit room queue");
    controller.addToQueue(4);
    check(controller.localQueue().contains(4LL) && !server.queue.contains(4), "default add without room permission uses local queue");
    controller.leaveRoom();
    check(waitUntil([&] { return controller.roomState().isEmpty() && controller.player()->song().value("id").toLongLong() == 1 && !controller.player()->loading(); }), "leave restores personal song");
    check(!controller.player()->playing() && controller.player()->position() >= 1000, "leave restores position paused");
    controller.createPlaylistWithSong("New playlist", 1);
    check(waitUntil([&] { return !controller.playlistCreationBusy() && !controller.playlists().isEmpty(); }), "new playlist created and song added through HTTP");
    check(server.playlists.first().toObject().value("songs").toArray().size() == 1, "created playlist contains selected song");
    controller.toggleFavorite(2);
    check(waitUntil([&] { return !controller.favoriteBusy() && controller.favoriteSongIds().contains(2LL); }), "heart adds actual favorite playlist entry");
    controller.toggleFavorite(2);
    check(waitUntil([&] { return !controller.favoriteBusy() && controller.favoriteSongIds().isEmpty(); }), "heart removes actual favorite entry");
    check(server.playlists.size() == 2, "repeated heart operation reuses existing favorite playlist");
    controller.createServerPlaylist("Public list");
    check(waitUntil([&] { return controller.serverPlaylists().size() == 1; }), "server playlist creation and list refresh use real HTTP");
    controller.addSongsToPlaylist(10, {1LL, 2LL}, true);
    check(waitUntil([&] { return controller.serverPlaylists().first().toMap().value("songs").toList().size() == 2; }), "server playlist adds songs and refreshes contents");
    controller.reorderPlaylist(10, {2LL, 1LL}, true);
    check(waitUntil([&] { return controller.serverPlaylists().first().toMap().value("songs").toList().first().toMap().value("id").toLongLong() == 2; }), "server playlist reorder persists on endpoint");
    controller.updatePlaylist(10, "Renamed public", "", "", true);
    check(waitUntil([&] { return controller.serverPlaylists().first().toMap().value("name") == "Renamed public"; }), "server playlist rename refreshes state");
    controller.removeSongFromPlaylist(10, 1, true);
    check(waitUntil([&] { return controller.serverPlaylists().first().toMap().value("songs").toList().size() == 1; }), "server playlist removes selected song");
    controller.deletePlaylist(10, true);
    check(waitUntil([&] { return controller.serverPlaylists().isEmpty(); }), "server playlist deletion refreshes navigation");
    server.control = true;
    controller.createRoom("Room");
    check(waitUntil([&] { return controller.roomOwner(); }), "creator may delete current room");
    controller.deleteRoom(7);
    check(waitUntil([&] { return !controller.roomDeletionBusy() && controller.roomState().isEmpty(); }), "room dissolve REST and WebSocket clear current room");
    check(controller.authenticated() && server.roomDeleteRequests == 1 && controller.localQueue() == QVariantList({1LL, 3LL, 2LL, 4LL}), "room dissolution keeps account and local queue");
    controller.createRoom("Room");
    check(waitUntil([&] { return controller.roomOwner(); }), "second room joins correctly");
    server.control = false; controller.refreshRoomState();
    check(waitUntil([&] { return !controller.roomOwner(); }), "non-owner cannot dissolve room");
    controller.deleteRoom(7); QTest::qWait(80);
    check(server.roomDeleteRequests == 1, "client refuses deletion without ownership");
    server.broadcast("ROOM_DELETED", QJsonObject{{"roomId", 99}}); QTest::qWait(80);
    check(!controller.roomState().isEmpty(), "unrelated room deletion does not clear current playback");
    server.activeRoom = false; server.roomExists = false;
    server.broadcast("ROOM_DELETED", QJsonObject{{"roomId", 7}});
    check(waitUntil([&] { return controller.roomState().isEmpty() && !controller.player()->loading(); }), "member exits dissolved room via notification");
    check(controller.authenticated() && !controller.player()->playing(), "dissolution notification restores personal playback paused");
    controller.clearError();

    server.control = true;
    controller.createRoom("Room");
    check(waitUntil([&] { return controller.roomOwner(); }), "creator rejoins own room");
    controller.leaveRoom();
    check(waitUntil([&] { return controller.roomState().isEmpty() && controller.canDeleteRoom(7); }), "creator retains dissolution permission after leaving");
    controller.deleteRoom(7);
    check(waitUntil([&] { return !controller.roomDeletionBusy() && server.roomDeleteRequests == 2; }), "creator dissolves room from public list after leaving");

    server.delayLyrics = true;
    const int lyricRequestsBefore = server.lyricRequests;
    controller.playSong(server.song(1).toVariantMap());
    check(controller.lyrics().isEmpty(), "song switch immediately clears previous lyrics");
    check(waitUntil([&] { return server.lyricRequests > lyricRequestsBefore; }), "first lyric request reaches server");
    controller.playSong(server.song(2).toVariantMap());
    check(waitUntil([&] { return controller.lyrics().value("songId").toLongLong() == 2; }), "new track lyrics loaded independently of page");
    QTest::qWait(500);
    check(controller.lyrics().value("lyrics").toString() == "[00:01.00]Lyrics 2", "late previous track response cannot replace current lyrics");
    server.delayLyrics = false;

    ApiClient editApi;
    editApi.setBaseUrl(QUrl(QString("http://127.0.0.1:%1/").arg(server.http.serverPort())));
    for (const bool put : {false, true}) {
        auto *parts = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        parts->setBoundary("boundary_/with+slash");
        QHttpPart field; field.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"title\"");
        field.setBody("multipart regression"); parts->append(field);
        bool done = false;
        if (put) editApi.putUpload("api/test-edit", parts, [&](const QJsonValue &) { done = true; });
        else editApi.upload("api/test-edit", parts, [&](const QJsonValue &) { done = true; });
        check(waitUntil([&] { return done; }) && server.multipartTypes.last() == "multipart/form-data; boundary=\"boundary_/with+slash\"", "POST and PUT quote Qt multipart boundaries on actual wire");
    }
    server.uploaded = {}; server.uploadBodies.clear(); server.multipartTypes.clear();

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"opacity", 0.0}});
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/imports");
    engine.rootContext()->setContextProperty("appController", &controller);
    int scriptErrors = 0;
    QObject::connect(&engine, &QQmlEngine::warnings, &app, [&](const QList<QQmlError> &warnings) { for (const auto &warning : warnings) { output << warning.toString() << Qt::endl; if (warning.description().contains("Error") || warning.description().contains("Unable to assign")) ++scriptErrors; } });
    engine.load(QUrl::fromLocalFile(QStringLiteral(FUTARI_SOURCE_DIR "/qml/Main.qml")));
    waitUntil([&] { return !engine.rootObjects().isEmpty(); });
    auto *window = engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!engine.rootObjects().isEmpty()) output << "Root type: " << engine.rootObjects().first()->metaObject()->className() << Qt::endl;
    check(window != nullptr, "full application QML loads");
    if (window) {
        QTest::qWait(600);
        auto grab = window->contentItem()->grabToImage();
        waitUntil([&] { return !grab->image().isNull(); });
        const QImage frame = grab->image();
        frame.save(QCoreApplication::applicationDirPath() + "/main-window.png");
        bool partialAlpha = false;
        for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) { const int alpha = frame.pixelColor(x, y).alpha(); if (alpha > 0 && alpha < 255) partialAlpha = true; }
        output << "Frame alpha: format=" << frame.format() << " alpha=" << frame.hasAlphaChannel() << " corner=" << frame.pixelColor(0, 0).alpha() << " center=" << frame.pixelColor(80, 80).alpha() << " partial=" << partialAlpha << Qt::endl;
        check(!frame.isNull() && frame.pixelColor(0, 0).alpha() == 0 && partialAlpha && frame.pixelColor(80, 80).alpha() == 255, "native rendering has transparent antialiased corners and opaque content");
        QQuickItem *row = nullptr;
        const auto items = visualItems(window->contentItem());
        for (auto *item : items) if (QString(item->metaObject()->className()).startsWith("RoomPage")) {
            auto roomPage = engine.newQObject(item);
            roomPage.property("confirmDelete").callWithInstance(roomPage, {7, "Room"});
            QTest::qWait(200);
            auto *dialog = item->findChild<QObject *>("deleteRoomDialog");
            auto *surface = dialog ? dialog->property("background").value<QQuickItem *>() : nullptr;
            check(dialog && dialog->property("visible").toBool() && dialog->property("height").toDouble() >= 180 &&
                  surface && surface->property("radius").toDouble() >= 16, "room dissolution confirmation has usable rounded layout");
            if (dialog) { QMetaObject::invokeMethod(dialog, "close"); QTest::qWait(200); }
            break;
        }
        for (auto *item : items) if (item->property("song").toMap().value("id").toLongLong() == 1 && item->property("actionsVisible").isValid()) { row = item; break; }
        check(row != nullptr, "real song row created");
        if (row) {
            const QPointF position = row->mapToScene(QPointF(180, 30));
            QTest::mouseMove(window, position.toPoint()); QTest::qWait(120);
            check(row->property("actionsVisible").toBool(), "hover reveals song actions");
            window->grabWindow().save(QCoreApplication::applicationDirPath() + "/song-hover.png");
            QObject *menu = row->findChild<QObject *>("songMoreMenu");
            check(menu != nullptr, "more menu available");
            if (menu) { menu->setProperty("x", 600); menu->setProperty("y", 200); QMetaObject::invokeMethod(menu, "open"); QTest::qWait(200); window->grabWindow().save(QCoreApplication::applicationDirPath() + "/song-menu.png"); QMetaObject::invokeMethod(menu, "close"); QTest::qWait(200); }
            QObject *addMenu = row->findChild<QObject *>("songAddMenu");
            check(addMenu != nullptr, "add menu available");
            if (addMenu) {
                addMenu->setProperty("x", 600); addMenu->setProperty("y", 200);
                QMetaObject::invokeMethod(addMenu, "open"); QTest::qWait(200);
                window->grabWindow().save(QCoreApplication::applicationDirPath() + "/song-add.png");
                QMetaObject::invokeMethod(addMenu, "close"); QTest::qWait(200);
            }
            controller.setDarkMode(true); QTest::qWait(200);
            if (menu) { QMetaObject::invokeMethod(menu, "open"); QTest::qWait(200); window->grabWindow().save(QCoreApplication::applicationDirPath() + "/song-menu-dark.png"); QMetaObject::invokeMethod(menu, "close"); QTest::qWait(200); }
            controller.setDarkMode(false); QTest::qWait(200);
            QMetaObject::invokeMethod(row, "requestNewPlaylist"); QTest::qWait(200);
            QObject *createDialog = nullptr;
            for (auto *child : row->findChildren<QObject *>()) if (child->property("title").toString() == QStringLiteral("创建新歌单")) { createDialog = child; break; }
            check(createDialog && createDialog->property("visible").toBool() && createDialog->property("height").toDouble() >= 170, "new playlist dialog opens with usable layout");
            if (createDialog) { window->grabWindow().save(QCoreApplication::applicationDirPath() + "/new-playlist.png"); QMetaObject::invokeMethod(createDialog, "close"); QTest::qWait(200); }
        }
        for (auto *item : items) if (QString(item->metaObject()->className()).startsWith("LibraryPage")) {
            for (const QString &id : {QStringLiteral("singleUpload"), QStringLiteral("batchUpload")}) {
                QObject *dialog = nullptr;
                for (auto *child : item->findChildren<QObject *>()) if (child->property("title").toString() == (id == "singleUpload" ? QStringLiteral("单曲上传") : QStringLiteral("批量上传歌曲"))) { dialog = child; break; }
                check(dialog != nullptr, id == "singleUpload" ? "single upload dialog available" : "batch upload dialog available");
                if (dialog) {
                    QMetaObject::invokeMethod(dialog, "open"); QTest::qWait(350);
                    auto *surface = dialog->property("background").value<QQuickItem *>();
                    check(dialog->property("visible").toBool() && surface && surface->property("radius").toDouble() >= 16, "upload surface is visible and rounded");
                    window->grabWindow().save(QCoreApplication::applicationDirPath() + "/" + id + ".png");
                    if (id == "batchUpload") {
                        auto urls = engine.newArray(2);
                        for (int i = 0; i < 2; ++i) {
                            const QString path = directory.path() + QString("/track-%1.wav").arg(i);
                            QFile audioFile(path); audioFile.open(QIODevice::WriteOnly); audioFile.write(server.audio); audioFile.close();
                            QFile lyricFile(directory.path() + QString("/track-%1.lrc").arg(i));
                            lyricFile.open(QIODevice::WriteOnly); lyricFile.write(QString("[00:01.00]Original %1").arg(i).toUtf8()); lyricFile.close();
                            urls.setProperty(i, QUrl::fromLocalFile(path).toString());
                        }
                        auto jsDialog = engine.newQObject(dialog);
                        jsDialog.property("addFiles").callWithInstance(jsDialog, {urls});
                        check(waitUntil([&] { return !dialog->property("parsing").toBool() && dialog->property("parseQueue").toList().isEmpty(); }), "batch metadata preprocessing completes");
                        auto *lyricEditor = dialog->findChild<QObject *>("batchLyricsEditor");
                        auto *model = dialog->findChild<QObject *>("batchUploadFiles");
                        check(model && lyricEditor, "batch per-track model and editor available");
                        if (model && lyricEditor) {
                            auto jsModel = engine.newQObject(model);
                            check(jsModel.property("get").callWithInstance(jsModel, {0}).property("lyricsText").toString() == "[00:01.00]Original 0" &&
                                  jsModel.property("get").callWithInstance(jsModel, {1}).property("lyricsText").toString() == "[00:01.00]Original 1", "matching associates separate sibling lyric files");
                            jsDialog.property("selectFile").callWithInstance(jsDialog, {0});
                            lyricEditor->setProperty("text", "[00:01.00]Edited A");
                            jsDialog.property("selectFile").callWithInstance(jsDialog, {1});
                            check(lyricEditor->property("text").toString() == "[00:01.00]Original 1", "selecting second song does not inherit first lyrics");
                            lyricEditor->setProperty("text", "[00:01.00]Edited B");
                            jsDialog.property("selectFile").callWithInstance(jsDialog, {0});
                            check(lyricEditor->property("text").toString() == "[00:01.00]Edited A", "first song keeps independent edited lyrics");
                            jsDialog.property("submitBatch").callWithInstance(jsDialog);
                            check(waitUntil([&] {
                                const auto tasks = controller.transferTasks();
                                return controller.uploadedSongs().size() == 2 && tasks.size() == 2 &&
                                        tasks[0].toMap().value("status") == "success" && tasks[1].toMap().value("status") == "success";
                            }), "batch uploads complete through actual network queue");
                            check(server.uploadBodies.size() == 2 && server.uploadBodies[0].contains("Edited A") && !server.uploadBodies[0].contains("Edited B") &&
                                  server.uploadBodies[1].contains("Edited B") && !server.uploadBodies[1].contains("Edited A"), "multipart upload snapshots keep each track lyrics separate");
                        }
                    }
                    QMetaObject::invokeMethod(dialog, "close"); QTest::qWait(200);
                }
            }
            break;
        }
        for (auto *item : visualItems(window->contentItem())) if (item->property("tabNames").toStringList().size() == 4) {
            auto page = engine.newQObject(item);
            item->setProperty("currentTab", 0);
            check(page.property("currentRows").callWithInstance(page).toVariant().toList().isEmpty(), "completed uploads are absent from active upload tab");
            item->setProperty("currentTab", 2);
            check(page.property("currentRows").callWithInstance(page).toVariant().toList().size() == 2, "completed uploads appear in server upload history");
            item->setProperty("currentTab", 0);
            server.failNextUpload = true;
            const QString failedTask = controller.enqueueUpload({{"audioUrl", QUrl::fromLocalFile(directory.path() + "/track-0.wav").toString()}, {"title", "Retry regression"}});
            check(waitUntil([&] { const auto rows = page.property("currentRows").callWithInstance(page).toVariant().toList(); return rows.size() == 1 && rows.first().toMap().value("status") == "failed"; }), "failed upload remains available for retry in active tab");
            controller.transferManager()->cancelTask(failedTask);
            check(page.property("currentRows").callWithInstance(page).toVariant().toList().isEmpty() && controller.transferTasks().size() == 2, "cancel removes failed task while preserving completed history");
            controller.setDownloadDirectory(directory.path() + "/downloads");
            controller.enqueueDownload(server.song(1).toVariantMap());
            check(waitUntil([&] { const auto tasks = controller.transferTasks(); return tasks.size() == 3 && tasks.last().toMap().value("status") == "success"; }), "real download completes and passes file hash validation");
            item->setProperty("currentTab", 1);
            check(page.property("currentRows").callWithInstance(page).toVariant().toList().isEmpty(), "completed downloads are absent from active download tab");
            item->setProperty("currentTab", 3);
            check(page.property("currentRows").callWithInstance(page).toVariant().toList().size() == 1, "completed download remains in download history");
            item->setProperty("currentTab", 0);
            break;
        }
    }
    if (argc > 1) {
        const QString path = QString::fromLocal8Bit(argv[1]);
        QFile source(path);
        check(source.open(QIODevice::ReadOnly), "user supplied audio exists");
        if (source.isOpen()) {
            const QByteArray bytes = source.readAll();
            const int before = server.uploaded.size();
            controller.enqueueUpload({{"audioUrl", QUrl::fromLocalFile(path).toString()}, {"title", "Actual FLAC regression"}, {"artist", "Test"}});
            check(waitUntil([&] { return server.uploaded.size() > before && controller.transferTasks().last().toMap().value("status") == "success"; }, 20000), "actual supplied FLAC transfers in full to test HTTP endpoint");
            check(!server.uploadBodies.isEmpty() && server.uploadBodies.last().contains(bytes), "actual FLAC payload is unchanged in multipart body");
        }
    }
    check(scriptErrors == 0, "UI interactions without QML script errors");
    controller.logout();
    return ok ? 0 : 1;
}
