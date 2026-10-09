#ifndef FUTARI_SESSION_STATE_H
#define FUTARI_SESSION_STATE_H

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QQueue>
#include <QSet>
#include <QVariantMap>

#include "../utils/RequestScope.h"
#include "PageState.h"

// 当前账号的内存快照，由 AppController 独占写入，QML 通过属性和信号观察。
// 退出登录时整体替换；RequestScope 的弱 token 随之失效，不允许旧响应回填新会话。
// 用户名、主题和自动登录偏好属于持久设置，不随此结构清空。
struct SessionState {
    PageState adminUsers;
    PageState userResults;
    PageState uploadedSongs;
    PageState rooms;
    PageState songs;
    // 服务端身份与权限；authenticated 仅在登录/恢复会话成功后置为 true。
    QString role;
    bool authenticated = false;
    bool canUpload = false;
    bool canManageServerPlaylist = false;
    qint64 userId = 0;
    // 0 表示本地播放；非零时 roomState 为服务端房间快照，UI 不自行推测房间状态。
    qint64 roomId = 0;
    // 0 表示空闲；请求期间记录目标房间，成功或失败后释放解散操作锁。
    qint64 deletingRoomId = 0;
    // 各数据源的 UI 快照，仅在对应请求仍有效时更新。
    QJsonArray albums;
    QJsonArray albumMatches;
    QJsonArray partners;
    QJsonArray playlists;
    QJsonArray serverPlaylists;
    QJsonArray invites;
    QJsonObject roomState;
    QJsonObject lyrics;
    RequestScope lyricsRequests;
    // 个人队列只保存歌曲 ID；进入房间不改变它，退出房间恢复 savedLocalSong/Position。
    QJsonArray localQueue;
    QJsonObject savedLocalSong;
    qint64 savedLocalPosition = 0;
    // 跨多个请求的收藏操作锁：确认歌单 → 创建/修改 → 刷新；任一失败均释放。
    bool favoriteBusy = false;
    // 创建歌单并添加歌曲是一条操作链，链结束前禁止重复提交。
    bool playlistCreationBusy = false;
    RequestScope playlistsRequests;
    RequestScope serverPlaylistsRequests;
    // 歌曲详情的统一索引；列表、歌单及单曲响应进入 UI 前必须写入。
    QHash<qint64, QJsonObject> songCache;
    // 封面 URL 与在途下载 ID；失效缓存时同时更新请求域，避免旧下载重新落盘。
    QVariantMap covers;
    QSet<QString> coverLoading;
    RequestScope coverRequests;
    // 专辑候选的最新搜索有效；与专辑列表刷新独立。
    RequestScope albumSearchRequests;
    RequestScope albumsRequests;
    // 本地单曲详情请求只服务最新的播放意图；进入房间时也必须使其失效。
    RequestScope localPlaybackRequests;
    // 批量删除串行执行；注销时整体替换会话并使当前响应失效。
    QQueue<qint64> pendingSongDeletions;
    RequestScope songDeletionRequests;
    bool deletingSongs = false;
    bool songDeletionChangedData = false;
    // 歌单移除串行执行，避免大批量选择时并发轰击服务端。
    QQueue<qint64> pendingPlaylistRemovals;
    RequestScope playlistRemovalRequests;
    qint64 playlistRemovalTargetId = 0;
    bool playlistRemovalTargetsServer = false;
    // 服务端时钟相对本机的偏差；首个有效 PONG 建立基准，后续样本平滑更新。
    qint64 clockOffsetMs = 0;
    bool clockSynced = false;
};

#endif  // FUTARI_SESSION_STATE_H
