#include <QDateTime>
#include <QRandomGenerator>

#include "AppController.h"

void AppController::playSong(const QVariantMap& song) {
    const QJsonObject data = QJsonObject::fromVariantMap(song);
    if (data.value("id").toInteger() <= 0) return;
    if (m_session.roomId && !canControl()) {
        setError(QStringLiteral("没有房间播放控制权"));
        return;
    }
    m_session.localPlaybackRequests.renew();
    m_session.songCache.insert(data.value("id").toInteger(), data);
    fetchCover(data);
    if (m_session.roomId) {
        if (canControl()) {
            QJsonArray ids = m_session.roomState.value("songIds").toArray();
            if (!ids.contains(data.value("id"))) {
                ids.append(data.value("id"));
                sendSocket("PLAYLIST_UPDATE", {{"songIds", ids}});
            }
            sendSocket("PLAY", {{"songId", data.value("id")}, {"positionMs", 0}});
        }
    } else {
        if (!m_session.localQueue.contains(data.value("id"))) {
            m_session.localQueue.append(data.value("id"));
            emit localQueueChanged();
        }
        m_player.playSong(data);
    }
}

void AppController::playSongs(const QVariantList& songIds) {
    if (songIds.isEmpty()) return;
    if (m_session.roomId && !canControl()) {
        setError(QStringLiteral("没有房间播放控制权"));
        return;
    }

    QJsonArray queue =
        m_session.roomId ? m_session.roomState.value("songIds").toArray() : m_session.localQueue;
    QSet<qint64> queuedIds;
    for (const auto& item : queue) queuedIds.insert(item.toInteger());
    qint64 firstSongId = 0;
    bool queueChanged = false;
    for (const QVariant& value : songIds) {
        const qint64 songId = value.toLongLong();
        if (songId <= 0) continue;
        if (firstSongId == 0) firstSongId = songId;
        if (queuedIds.contains(songId)) continue;
        queue.append(songId);
        queuedIds.insert(songId);
        queueChanged = true;
    }
    if (firstSongId <= 0) return;

    if (m_session.roomId) {
        if (queueChanged) sendSocket("PLAYLIST_UPDATE", {{"songIds", queue}});
        sendSocket("PLAY", {{"songId", firstSongId}, {"positionMs", 0}});
        return;
    }

    if (queueChanged) {
        m_session.localQueue = queue;
        emit localQueueChanged();
    }
    playLocalById(firstSongId);
}

void AppController::togglePlayback() {
    if (m_session.roomId) {
        if (!canControl()) return;
        sendSocket(m_player.playing() ? "PAUSE" : "PLAY",
                   {{"songId", m_player.song().value("id").toLongLong()},
                    {"positionMs", m_player.position()}});
    } else if (m_player.playing())
        m_player.pause();
    else
        m_player.resume();
}

void AppController::seek(qint64 positionMs) {
    if (m_session.roomId) {
        if (canControl()) sendSocket("SEEK", {{"positionMs", positionMs}});
    } else
        m_player.seek(positionMs);
}

void AppController::nextSong() {
    if (m_session.roomId) {
        if (canControl()) sendSocket("NEXT", {});
        return;
    }
    advanceLocalQueue(AdvanceReason::Manual);
}

QString AppController::playbackMode() const {
    return m_session.roomId ? m_session.roomState.value("playback")
                                  .toObject()
                                  .value("mode")
                                  .toString("SEQUENTIAL")
                            : m_settings.value("playbackMode", "SEQUENTIAL").toString();
}

void AppController::setPlaybackMode(const QString& mode) {
    if (mode != "SEQUENTIAL" && mode != "REPEAT_ONE" && mode != "SHUFFLE") return;
    if (m_session.roomId) {
        // 房间模式以服务端广播为准；离开房间仍保留原来的本地偏好。
        if (canControl()) sendSocket("PLAYBACK_MODE", {{"mode", mode}});
        return;
    }
    m_settings.setValue("playbackMode", mode);
    emit playbackModeChanged();
}

void AppController::onTrackEnded() {
    if (!m_session.roomId) {
        advanceLocalQueue(AdvanceReason::TrackEnded);
        return;
    }
    if (!roomOwner()) return;
    const auto playback = m_session.roomState.value("playback").toObject();
    sendSocket("TRACK_ENDED", {{"songId", m_player.song().value("id").toLongLong()},
                               {"timestamp", playback.value("serverTimestamp")}});
}

void AppController::advanceLocalQueue(AdvanceReason reason) {
    const QJsonArray& ids = m_session.localQueue;
    if (ids.isEmpty()) return;
    const qint64 current = m_player.song().value("id").toLongLong();
    int index = -1;
    for (int i = 0; i < ids.size(); ++i)
        if (ids.at(i).toInteger() == current) index = i;
    if (reason == AdvanceReason::TrackEnded && playbackMode() == "REPEAT_ONE" && index >= 0) {
        playLocalById(current);
        return;
    }
    if (playbackMode() == "SHUFFLE" && ids.size() > 1) {
        const int choices = index < 0 ? ids.size() : ids.size() - 1;
        const int offset = QRandomGenerator::global()->bounded(choices);
        playLocalById(ids.at(index < 0 ? offset : (index + 1 + offset) % ids.size()).toInteger());
        return;
    }
    // 顺序播放自然结束后停止；手动下一首允许回到队首。
    if (reason == AdvanceReason::TrackEnded && index == ids.size() - 1) return;
    playLocalById(ids.at((index + 1) % ids.size()).toInteger());
}

void AppController::moveQueueBefore(qint64 songId, qint64 beforeId) {
    if (songId == beforeId || (m_session.roomId && !canControl())) return;
    QJsonArray ids =
        m_session.roomId ? m_session.roomState.value("songIds").toArray() : m_session.localQueue;
    if (!ids.contains(songId) || (beforeId && !ids.contains(beforeId))) return;
    if (m_session.roomId) {
        // 发送 ID 相对移动，由服务端合并当前队列，避免拖动期间的新增歌曲被覆盖。
        sendSocket("QUEUE_MOVE", {{"songId", songId}, {"beforeId", beforeId}});
        return;
    }
    for (int i = 0; i < ids.size(); ++i)
        if (ids.at(i).toInteger() == songId) {
            ids.removeAt(i);
            break;
        }
    int target = ids.size();
    for (int i = 0; i < ids.size(); ++i)
        if (ids.at(i).toInteger() == beforeId) target = i;
    ids.insert(target, songId);
    m_session.localQueue = ids;
    emit localQueueChanged();
}

void AppController::previousSong() {
    const qint64 current = m_player.song().value("id").toLongLong();
    if (m_session.roomId) {
        if (!canControl()) return;
        const QJsonArray ids = m_session.roomState.value("songIds").toArray();
        for (int index = 1; index < ids.size(); ++index) {
            if (ids.at(index).toInteger() == current) {
                sendSocket("PLAY", {{"songId", ids.at(index - 1)}, {"positionMs", 0}});
                return;
            }
        }
        return;
    }
    for (int index = 1; index < m_session.localQueue.size(); ++index) {
        if (m_session.localQueue.at(index).toInteger() == current) {
            playLocalById(m_session.localQueue.at(index - 1).toInteger());
            return;
        }
    }
}

void AppController::playLocalById(qint64 id) {
    const auto token = m_session.localPlaybackRequests.renew();
    const QJsonObject song = findSong(id);
    if (!song.isEmpty()) {
        m_player.playSong(song);
        return;
    }
    m_api.request("GET", "api/songs/" + QString::number(id), {},
                  [this, token](const QJsonValue& value) {
                      if (token.expired() || m_session.roomId) return;
                      const QJsonObject song = value.toObject();
                      m_session.songCache.insert(song.value("id").toInteger(), song);
                      emit songsChanged();
                      fetchCover(song);
                      m_player.playSong(song);
                  });
}

void AppController::addToQueue(qint64 songId) {
    if (m_session.roomId && canControl())
        addToRoomQueue(songId);
    else
        addToLocalQueue(songId);
}

void AppController::addToLocalQueue(qint64 songId) {
    if (songId <= 0) return;
    if (!m_session.localQueue.contains(songId)) {
        m_session.localQueue.append(songId);
        emit localQueueChanged();
        emit infoMessage(QStringLiteral("已加入本地播放队列"));
    } else
        emit infoMessage(QStringLiteral("歌曲已在本地播放队列中"));
}

void AppController::addToRoomQueue(qint64 songId) {
    if (songId <= 0) return;
    if (!m_session.roomId || !canControl()) {
        setError(QStringLiteral("需要加入房间并获得播放控制权"));
        return;
    }
    QJsonArray ids = m_session.roomState.value("songIds").toArray();
    ids.append(songId);
    if (m_session.roomState.value("songIds").toArray().contains(songId)) return;
    publishQueue(ids);
}

void AppController::addSongsToQueue(const QVariantList& songIds) {
    if (songIds.isEmpty()) return;
    if (!m_session.roomId || !canControl()) {
        addSongsToLocalQueue(songIds);
        return;
    }

    QJsonArray ids = m_session.roomState.value("songIds").toArray();
    QSet<qint64> queuedIds;
    for (const auto& value : ids) queuedIds.insert(value.toInteger());
    bool changed = false;
    for (const QVariant& value : songIds) {
        const qint64 songId = value.toLongLong();
        if (songId <= 0 || queuedIds.contains(songId)) continue;
        ids.append(songId);
        queuedIds.insert(songId);
        changed = true;
    }
    if (changed) publishQueue(ids);
}

void AppController::addSongsToLocalQueue(const QVariantList& songIds) {
    bool changed = false;
    for (const QVariant& value : songIds) {
        const qint64 songId = value.toLongLong();
        if (songId <= 0 || m_session.localQueue.contains(songId)) continue;
        m_session.localQueue.append(songId);
        changed = true;
    }
    if (changed) {
        emit localQueueChanged();
        emit infoMessage(QStringLiteral("已加入本地播放队列"));
    }
}

void AppController::playNext(const QVariantMap& song) {
    const QJsonObject data = QJsonObject::fromVariantMap(song);
    const qint64 songId = data.value("id").toInteger();
    if (songId <= 0) return;
    if (m_session.roomId && !canControl()) {
        setError(QStringLiteral("没有房间播放控制权"));
        return;
    }
    const qint64 current =
        m_session.roomId
            ? m_session.roomState.value("playback").toObject().value("currentSongId").toInteger()
            : m_player.song().value("id").toLongLong();
    if (current == songId) {
        emit infoMessage(QStringLiteral("这首歌曲正在播放"));
        return;
    }
    m_session.songCache.insert(songId, data);
    fetchCover(data);
    QJsonArray ids =
        m_session.roomId ? m_session.roomState.value("songIds").toArray() : m_session.localQueue;
    for (int i = ids.size() - 1; i >= 0; --i)
        if (ids.at(i).toInteger() == songId) ids.removeAt(i);
    int insertion = 0;
    for (int i = 0; i < ids.size(); ++i)
        if (ids.at(i).toInteger() == current) {
            insertion = i + 1;
            break;
        }
    ids.insert(insertion, songId);
    if (m_session.roomId)
        publishQueue(ids);
    else {
        m_session.localQueue = ids;
        emit localQueueChanged();
        emit infoMessage(QStringLiteral("已设为下一首播放"));
    }
}

void AppController::publishQueue(const QJsonArray& ids) {
    if (m_session.roomId && canControl()) sendSocket("PLAYLIST_UPDATE", {{"songIds", ids}});
}

void AppController::moveQueueSong(int index, int direction) {
    if (m_session.roomId && !canControl()) return;
    const QJsonArray ids =
        m_session.roomId ? m_session.roomState.value("songIds").toArray() : m_session.localQueue;
    const int target = index + direction;
    if (index < 0 || index >= ids.size() || target < 0 || target >= ids.size()) return;
    const int before = target > index ? target + 1 : target;
    moveQueueBefore(ids.at(index).toInteger(), before < ids.size() ? ids.at(before).toInteger() : 0);
}

void AppController::removeQueueSong(int index) {
    if (m_session.roomId && !canControl()) return;
    QJsonArray ids =
        m_session.roomId ? m_session.roomState.value("songIds").toArray() : m_session.localQueue;
    if (index < 0 || index >= ids.size()) return;
    ids.removeAt(index);
    if (m_session.roomId)
        publishQueue(ids);
    else {
        m_session.localQueue = ids;
        emit localQueueChanged();
    }
}

void AppController::loadLyrics(qint64 songId) {
    const auto token = m_session.lyricsRequests.renew();
    m_session.lyrics = {};
    emit lyricsChanged();
    if (songId <= 0 || !authenticated()) return;
    m_api.request("GET", "api/songs/" + QString::number(songId) + "/lyrics", {},
                  [this, songId, token](const QJsonValue& value) {
                      if (token.expired() || m_player.song().value("id").toLongLong() != songId)
                          return;
                      const QJsonObject result = value.toObject();
                      if (result.value("songId").toInteger() != songId) return;
                      m_session.lyrics = result;
                      emit lyricsChanged();
                  });
}

void AppController::applyPlayback(const QJsonObject& playback) {
    if (!m_session.roomId) return;
    const qint64 songId = playback.value("currentSongId").toInteger();
    if (!songId) {
        m_player.stop();
        return;
    }
    const QJsonObject song = findSong(songId);
    if (song.isEmpty()) {
        const qint64 roomId = m_session.roomId;
        m_api.request("GET", "api/songs/" + QString::number(songId), {},
                      [this, playback, roomId](const QJsonValue& value) {
                          if (m_session.roomId != roomId ||
                              m_session.roomState.value("playback").toObject() != playback)
                              return;
                          const QJsonObject song = value.toObject();
                          m_session.songCache.insert(song.value("id").toInteger(), song);
                          emit songsChanged();
                          fetchCover(song);
                          applyPlayback(playback);
                      });
        return;
    }
    fetchCover(song);
    qint64 position = playback.value("positionMs").toInteger();
    const bool playing = playback.value("status").toString() == "playing";
    if (playing)
        position += qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() + m_session.clockOffsetMs -
                                        playback.value("serverTimestamp").toInteger());
    if (m_player.song().value("id").toLongLong() != songId)
        m_player.playSong(song, position, playing);
    else {
        if (qAbs(m_player.position() - position) > 500) m_player.seek(position);
        if (playing && !m_player.playing())
            m_player.resume();
        else if (!playing && m_player.playing())
            m_player.pause();
    }
}

bool AppController::isStalePlayback(const QJsonObject& playback) const {
    const auto current = m_session.roomState.value("playback").toObject();
    // 模式变化保留播放基准时间，因此不能仅通过时间戳比较快照的新旧。
    return current.value("revision").toInteger() > playback.value("revision").toInteger() ||
           current.value("serverTimestamp").toInteger() > playback.value("serverTimestamp").toInteger();
}
