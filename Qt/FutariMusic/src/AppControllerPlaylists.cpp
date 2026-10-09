#include "AppController.h"

void AppController::refreshPlaylists() {
    if (!authenticated()) return;
    const auto token = m_session.playlistsRequests.renew();
    m_api.request("GET", "api/playlists", {}, [this, token](const QJsonValue& value) {
        if (token.expired()) return;
        m_session.playlists = value.toArray();
        cachePlaylistSongs(m_session.playlists);
        emit playlistsChanged();
    });
}

void AppController::createPlaylist(const QString& name) {
    m_api.request("POST", "api/playlists", {{"name", name.trimmed()}, {"description", ""}},
                  [this](const QJsonValue&) { refreshPlaylists(); });
}

void AppController::createPlaylistWithSong(const QString& name, qint64 songId) {
    const QString normalized = name.trimmed();
    if (!authenticated() || normalized.isEmpty() || songId <= 0 || m_session.playlistCreationBusy)
        return;
    m_session.playlistCreationBusy = true;
    emit playlistCreationBusyChanged();
    m_api.request(
        "POST", "api/playlists", {{"name", normalized}, {"description", ""}},
        [this, songId](const QJsonValue& value) {
            const qint64 id = value.toObject().value("id").toInteger();
            if (id <= 0) {
                setError(QStringLiteral("服务器未返回新歌单编号"));
                finishPlaylistCreation(songId, SongAddition::Failed);
                return;
            }
            addCreatedPlaylistSong(id, songId);
        },
        [this, songId] { finishPlaylistCreation(songId, SongAddition::Failed); });
}

void AppController::addCreatedPlaylistSong(qint64 playlistId, qint64 songId) {
    m_api.request(
        "POST", "api/playlists/" + QString::number(playlistId) + "/songs",
        {{"songIds", QJsonArray{songId}}},
        [this, songId](const QJsonValue&) {
            refreshPlaylists();
            finishPlaylistCreation(songId, SongAddition::Added);
            emit infoMessage(QStringLiteral("已创建歌单并添加歌曲"));
        },
        [this, songId] {
            // 创建已成功：即使加歌失败也刷新列表，让用户能在新歌单里重试。
            refreshPlaylists();
            finishPlaylistCreation(songId, SongAddition::Failed);
            emit infoMessage(QStringLiteral("歌单已创建，歌曲添加失败，可在歌单页重试"));
        });
}

void AppController::finishPlaylistCreation(qint64 songId, SongAddition result) {
    m_session.playlistCreationBusy = false;
    emit playlistCreationBusyChanged();
    emit playlistCreationFinished(songId, result == SongAddition::Added);
}

void AppController::createPlaylistWithSongs(const QString& name, const QVariantList& songIds) {
    const QString normalized = name.trimmed();
    if (!authenticated() || normalized.isEmpty() || songIds.isEmpty() ||
        m_session.playlistCreationBusy)
        return;
    m_session.playlistCreationBusy = true;
    emit playlistCreationBusyChanged();
    m_api.request(
        "POST", "api/playlists", {{"name", normalized}, {"description", ""}},
        [this, songIds](const QJsonValue& value) {
            const qint64 playlistId = value.toObject().value("id").toInteger();
            if (playlistId <= 0) {
                setError(QStringLiteral("服务器未返回新歌单编号"));
                finishPlaylistCreation(songIds, SongAddition::Failed);
                return;
            }
            addCreatedPlaylistSongs(playlistId, songIds);
        },
        [this, songIds] { finishPlaylistCreation(songIds, SongAddition::Failed); });
}

void AppController::addCreatedPlaylistSongs(qint64 playlistId, const QVariantList& songIds) {
    QJsonArray ids;
    for (const QVariant& value : songIds) {
        const qint64 id = value.toLongLong();
        if (id > 0 && !ids.contains(id)) ids.append(id);
    }
    m_api.request(
        "POST", "api/playlists/" + QString::number(playlistId) + "/songs", {{"songIds", ids}},
        [this, songIds](const QJsonValue&) {
            refreshPlaylists();
            finishPlaylistCreation(songIds, SongAddition::Added);
            emit infoMessage(QStringLiteral("已创建歌单并添加所选歌曲"));
        },
        [this, songIds] {
            refreshPlaylists();
            finishPlaylistCreation(songIds, SongAddition::Failed);
            emit infoMessage(QStringLiteral("歌单已创建，添加歌曲失败，可在歌单页重试"));
        });
}

void AppController::finishPlaylistCreation(const QVariantList& songIds, SongAddition result) {
    m_session.playlistCreationBusy = false;
    emit playlistCreationBusyChanged();
    for (const QVariant& value : songIds)
        emit playlistCreationFinished(value.toLongLong(), result == SongAddition::Added);
}

qint64 AppController::favoritePlaylistId() const {
    for (const auto& entry : m_session.playlists) {
        const QJsonObject playlist = entry.toObject();
        if (playlist.value("name").toString() == QStringLiteral("我喜欢"))
            return playlist.value("id").toInteger();
    }
    return 0;
}

QVariantList AppController::favoriteSongIds() const {
    QVariantList ids;
    const qint64 favoriteId = favoritePlaylistId();
    for (const auto& entry : m_session.playlists) {
        const QJsonObject playlist = entry.toObject();
        if (playlist.value("id").toInteger() != favoriteId) continue;
        for (const auto& song : playlist.value("songs").toArray())
            ids.append(song.toObject().value("id").toInteger());
        break;
    }
    return ids;
}

void AppController::toggleFavorite(qint64 songId) {
    if (!authenticated() || songId <= 0 || m_session.favoriteBusy) return;
    m_session.favoriteBusy = true;
    emit playlistsChanged();
    // 先确认服务端歌单，不能把尚未加载的缓存当作“没有收藏歌单”。
    const auto token = m_session.playlistsRequests.renew();
    m_api.request(
        "GET", "api/playlists", {},
        [this, songId, token](const QJsonValue& value) {
            if (token.expired()) {
                finishFavorite();
                return;
            }
            m_session.playlists = value.toArray();
            cachePlaylistSongs(m_session.playlists);
            emit playlistsChanged();
            prepareFavoriteUpdate(songId);
        },
        [this] { finishFavorite(); });
}

void AppController::finishFavorite() {
    m_session.favoriteBusy = false;
    emit playlistsChanged();
}

void AppController::prepareFavoriteUpdate(qint64 songId) {
    const qint64 playlistId = favoritePlaylistId();
    if (playlistId > 0) {
        const auto change = favoriteSongIds().contains(QVariant::fromValue(songId))
                                ? FavoriteChange::Remove
                                : FavoriteChange::Add;
        updateFavorite(playlistId, songId, change);
        return;
    }
    m_api.request(
        "POST", "api/playlists", {{"name", QStringLiteral("我喜欢")}, {"description", ""}},
        [this, songId](const QJsonValue& value) {
            const QJsonObject playlist = value.toObject();
            const qint64 id = playlist.value("id").toInteger();
            if (id <= 0) {
                setError(QStringLiteral("服务器未返回歌单编号"));
                finishFavorite();
                return;
            }
            m_session.playlists.append(playlist);
            emit playlistsChanged();
            updateFavorite(id, songId, FavoriteChange::Add);
        },
        [this] { finishFavorite(); });
}

void AppController::updateFavorite(qint64 playlistId, qint64 songId, FavoriteChange change) {
    const bool remove = change == FavoriteChange::Remove;
    const QString path = "api/playlists/" + QString::number(playlistId) + "/songs" +
                         (remove ? "/" + QString::number(songId) : "");
    const QJsonObject body = remove ? QJsonObject{} : QJsonObject{{"songIds", QJsonArray{songId}}};
    m_api.request(
        remove ? "DELETE" : "POST", path, body,
        [this, change](const QJsonValue&) { reloadFavorite(change); },
        [this] { finishFavorite(); });
}

void AppController::reloadFavorite(FavoriteChange change) {
    const auto token = m_session.playlistsRequests.renew();
    m_api.request(
        "GET", "api/playlists", {},
        [this, change, token](const QJsonValue& value) {
            if (token.expired()) {
                finishFavorite();
                return;
            }
            m_session.playlists = value.toArray();
            cachePlaylistSongs(m_session.playlists);
            finishFavorite();
            emit infoMessage(change == FavoriteChange::Remove ? QStringLiteral("已取消喜欢")
                                                              : QStringLiteral("已添加到我喜欢"));
        },
        [this] { finishFavorite(); });
}

void AppController::updatePlaylist(qint64 playlistId, const QString& name,
                                   const QString& description, const QString& coverUrl,
                                   bool server) {
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) {
        setError(QStringLiteral("歌单名称不能为空"));
        return;
    }
    if (server && !canManageServerPlaylist()) {
        setError(QStringLiteral("当前账号没有服务器歌单管理权限"));
        return;
    }
    const QString path =
        (server ? "api/server-playlists/" : "api/playlists/") + QString::number(playlistId);
    m_api.request("PUT", path,
                  {{"name", normalizedName}, {"description", description}, {"coverUrl", coverUrl}},
                  [this, server](const QJsonValue&) {
                      if (server)
                          refreshServerPlaylists();
                      else
                          refreshPlaylists();
                      emit infoMessage(QStringLiteral("歌单名称已保存"));
                  });
}

void AppController::addSongToPlaylist(qint64 playlistId, qint64 songId) {
    addSongsToPlaylist(playlistId, {QVariant::fromValue(songId)}, false);
}

void AppController::addSongsToPlaylist(qint64 playlistId, const QVariantList& songIds,
                                       bool server) {
    if (songIds.isEmpty()) return;
    if (server && !canManageServerPlaylist()) {
        setError(QStringLiteral("当前账号没有服务器歌单管理权限"));
        return;
    }
    QJsonArray ids;
    for (const QVariant& songId : songIds) ids.append(songId.toLongLong());
    const QString path = (server ? "api/server-playlists/" : "api/playlists/") +
                         QString::number(playlistId) + "/songs";
    m_api.request("POST", path, {{"songIds", ids}}, [this, server](const QJsonValue& value) {
        if (server)
            refreshServerPlaylists();
        else
            refreshPlaylists();
        const QJsonObject result = value.toObject();
        emit infoMessage(QStringLiteral("成功加入 %1 首（%2 首已存在）")
                             .arg(result.value("added").toInteger())
                             .arg(result.value("duplicated").toInteger()));
    });
}

void AppController::refreshServerPlaylists() {
    if (!authenticated()) return;
    const auto token = m_session.serverPlaylistsRequests.renew();
    m_api.request("GET", "api/server-playlists", {}, [this, token](const QJsonValue& value) {
        if (token.expired()) return;
        m_session.serverPlaylists = value.toArray();
        cachePlaylistSongs(m_session.serverPlaylists);
        emit serverPlaylistsChanged();
    });
}

void AppController::createServerPlaylist(const QString& name) {
    if (!canManageServerPlaylist() || name.trimmed().isEmpty()) return;
    m_api.request("POST", "api/server-playlists", {{"name", name.trimmed()}, {"description", ""}},
                  [this](const QJsonValue&) { refreshServerPlaylists(); });
}

void AppController::addSongToServerPlaylist(qint64 playlistId, qint64 songId) {
    addSongsToPlaylist(playlistId, {QVariant::fromValue(songId)}, true);
}

void AppController::deletePlaylist(qint64 playlistId, bool server) {
    if (server && !canManageServerPlaylist()) return;
    m_api.request(
        "DELETE",
        (server ? "api/server-playlists/" : "api/playlists/") + QString::number(playlistId), {},
        [this, server](const QJsonValue&) {
            if (server)
                refreshServerPlaylists();
            else
                refreshPlaylists();
        });
}

void AppController::removeSongFromPlaylist(qint64 playlistId, qint64 songId, bool server) {
    if (server && !canManageServerPlaylist()) return;
    m_api.request("DELETE",
                  (server ? "api/server-playlists/" : "api/playlists/") +
                      QString::number(playlistId) + "/songs/" + QString::number(songId),
                  {}, [this, server](const QJsonValue&) {
                      if (server)
                          refreshServerPlaylists();
                      else
                          refreshPlaylists();
                  });
}

void AppController::removeSongsFromPlaylist(qint64 playlistId, const QVariantList& songIds,
                                            bool server) {
    if (songIds.isEmpty()) return;
    if (server && !canManageServerPlaylist()) {
        setError(QStringLiteral("当前账号没有服务器歌单管理权限"));
        return;
    }
    if (!m_session.pendingPlaylistRemovals.isEmpty()) {
        setError(QStringLiteral("请等待当前歌单移除操作完成"));
        return;
    }
    QSet<qint64> uniqueIds;
    for (const QVariant& value : songIds) {
        const qint64 id = value.toLongLong();
        if (id > 0) uniqueIds.insert(id);
    }
    for (qint64 id : uniqueIds) m_session.pendingPlaylistRemovals.enqueue(id);
    if (m_session.pendingPlaylistRemovals.isEmpty()) return;
    m_session.playlistRemovalTargetId = playlistId;
    m_session.playlistRemovalTargetsServer = server;
    removeNextPlaylistSong(m_session.playlistRemovalRequests.renew());
}

void AppController::removeNextPlaylistSong(const RequestScope::Token& token) {
    if (token.expired()) return;
    if (m_session.pendingPlaylistRemovals.isEmpty()) {
        if (m_session.playlistRemovalTargetsServer)
            refreshServerPlaylists();
        else
            refreshPlaylists();
        m_session.playlistRemovalTargetId = 0;
        return;
    }

    const qint64 songId = m_session.pendingPlaylistRemovals.dequeue();
    const QString basePath =
        m_session.playlistRemovalTargetsServer ? "api/server-playlists/" : "api/playlists/";
    const QString path = basePath + QString::number(m_session.playlistRemovalTargetId) + "/songs/" +
                         QString::number(songId);
    m_api.request(
        "DELETE", path, {},
        [this, token](const QJsonValue&) {
            if (!token.expired()) removeNextPlaylistSong(token);
        },
        [this, token] {
            if (token.expired()) return;
            m_session.pendingPlaylistRemovals.clear();
            removeNextPlaylistSong(token);
        });
}

void AppController::reorderPlaylist(qint64 playlistId, const QVariantList& songIds, bool server) {
    if (server && !canManageServerPlaylist()) return;
    QJsonArray ids;
    for (const QVariant& id : songIds) ids.append(id.toLongLong());
    m_api.request("PUT",
                  (server ? "api/server-playlists/" : "api/playlists/") +
                      QString::number(playlistId) + "/order",
                  {{"songIds", ids}}, [this, server](const QJsonValue&) {
                      if (server)
                          refreshServerPlaylists();
                      else
                          refreshPlaylists();
                  });
}
