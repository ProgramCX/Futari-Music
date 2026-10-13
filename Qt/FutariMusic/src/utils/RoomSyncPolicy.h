#ifndef FUTARI_ROOM_SYNC_POLICY_H
#define FUTARI_ROOM_SYNC_POLICY_H

#include <QJsonObject>

namespace RoomSyncPolicy {
inline bool isPlaybackCommand(const QString& type) {
    return type == "PLAY" || type == "PAUSE" || type == "SEEK" || type == "NEXT" ||
           type == "TRACK_ENDED";
}

inline bool sameTimeline(const QJsonObject &left, const QJsonObject &right) {
    if (left.isEmpty() || right.isEmpty()) return false;
    // Queue/mode revisions do not start a new playback timeline.
    for (const char *key : {"currentSongId", "status", "positionMs", "serverTimestamp"})
        if (left.value(key) != right.value(key)) return false;
    return true;
}

inline bool shouldApplySnapshot(const QJsonObject& applied, const QJsonObject& incoming) {
    // Restore on entry/reconnect; queue deletion may change or clear the current song.
    // Ordinary member/metadata refreshes must never reposition the same song.
    return !incoming.isEmpty() &&
           (applied.isEmpty() || applied.value("currentSongId") != incoming.value("currentSongId"));
}
}

#endif
