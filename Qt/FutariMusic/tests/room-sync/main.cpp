#include "../../src/utils/RoomSyncPolicy.h"
#include <QDebug>

int main() {
    int failures = 0;
    const auto check = [&](bool ok, const char *name) {
        if (!ok) { qCritical() << name; ++failures; }
    };
    const QJsonObject play{{"currentSongId", 119}, {"status", "playing"},
                           {"positionMs", 0}, {"serverTimestamp", 100000}, {"revision", 1}};
    auto metadataUpdate = play;
    metadataUpdate.insert("revision", 2);
    metadataUpdate.insert("mode", "RANDOM");
    check(RoomSyncPolicy::sameTimeline(play, metadataUpdate), "mode/member refresh must not seek");
    auto missedSeek = metadataUpdate;
    missedSeek.insert("positionMs", 45000);
    missedSeek.insert("serverTimestamp", 115000);
    check(!RoomSyncPolicy::sameTimeline(play, missedSeek), "explicit SEEK starts a new timeline");
    check(!RoomSyncPolicy::sameTimeline({}, play), "first room snapshot is a playback command");
    for (const auto* type : {"SYNC", "PLAYBACK_MODE", "MEMBER_JOIN", "MEMBER_LEAVE", "CONTROL_UPDATE"})
        check(!RoomSyncPolicy::isPlaybackCommand(type), "background notifications must not control audio");
    for (const auto* type : {"PLAY", "PAUSE", "SEEK", "NEXT", "TRACK_ENDED"})
        check(RoomSyncPolicy::isPlaybackCommand(type), "explicit controls and automatic next remain functional");
    check(!RoomSyncPolicy::shouldApplySnapshot(play, metadataUpdate), "metadata refresh must not seek");
    check(!RoomSyncPolicy::shouldApplySnapshot(play, missedSeek), "REST must not reintroduce periodic correction");
    check(RoomSyncPolicy::shouldApplySnapshot({}, missedSeek), "reconnect restores missed commands once");
    check(!RoomSyncPolicy::shouldApplySnapshot({}, {}), "empty snapshot must not stop audio");
    auto nextSong = missedSeek;
    nextSong.insert("currentSongId", 120);
    check(RoomSyncPolicy::shouldApplySnapshot(play, nextSong), "queue deletion can select another song");
    nextSong.insert("currentSongId", QJsonValue::Null);
    check(RoomSyncPolicy::shouldApplySnapshot(play, nextSong), "deleting the final song must stop playback");
    return failures ? 1 : 0;
}
