/****************************************************************************
** Meta object code from reading C++ file 'AppController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../Qt/FutariMusic/src/AppController.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AppController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13AppControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13AppControllerE = QtMocHelpers::stringData(
    "AppController",
    "playbackModeChanged",
    "",
    "sessionChanged",
    "autoLoginChanged",
    "serverUrlChanged",
    "darkModeChanged",
    "errorMessageChanged",
    "songsChanged",
    "albumsChanged",
    "albumMatchesChanged",
    "transferTasksChanged",
    "uploadedSongsChanged",
    "settingsChanged",
    "roomsChanged",
    "partnersChanged",
    "userResultsChanged",
    "playlistsChanged",
    "serverPlaylistsChanged",
    "invitesChanged",
    "roomStateChanged",
    "roomLeaveForCloseFinished",
    "localQueueChanged",
    "lyricsChanged",
    "roomDeletionBusyChanged",
    "coversChanged",
    "adminUsersChanged",
    "invitationArrived",
    "title",
    "message",
    "openMessagesRequested",
    "infoMessage",
    "songSaved",
    "songDeleted",
    "audioMetadataReady",
    "requestId",
    "QVariantMap",
    "metadata",
    "playlistCreationBusyChanged",
    "playlistCreationFinished",
    "songId",
    "added",
    "login",
    "username",
    "password",
    "registerAccount",
    "nickname",
    "logout",
    "clearError",
    "refreshAll",
    "searchSongs",
    "keyword",
    "loadMoreSongs",
    "refreshAlbums",
    "refreshUploadedSongs",
    "loadMoreUploadedSongs",
    "enqueueUpload",
    "enqueueDownload",
    "song",
    "openLocalFolder",
    "path",
    "clearSongCache",
    "searchAlbumMatches",
    "name",
    "artist",
    "inspectAudio",
    "audioUrl",
    "findLyricsForAudio",
    "readLyricsFile",
    "lyricsUrl",
    "classifyLyrics",
    "text",
    "parseLyrics",
    "QVariantList",
    "countLikelyCreditLines",
    "refreshRooms",
    "loadMoreRooms",
    "createRoom",
    "joinRoom",
    "id",
    "leaveRoom",
    "leaveRoomBeforeClose",
    "canDeleteRoom",
    "roomId",
    "deleteRoom",
    "refreshRoomState",
    "setController",
    "memberId",
    "enabled",
    "playSong",
    "playSongs",
    "songIds",
    "togglePlayback",
    "seek",
    "positionMs",
    "nextSong",
    "setPlaybackMode",
    "mode",
    "moveQueueBefore",
    "beforeId",
    "previousSong",
    "addToQueue",
    "addToLocalQueue",
    "addToRoomQueue",
    "addSongsToQueue",
    "addSongsToLocalQueue",
    "playNext",
    "toggleFavorite",
    "createPlaylistWithSong",
    "createPlaylistWithSongs",
    "moveQueueSong",
    "index",
    "direction",
    "removeQueueSong",
    "removeQueueSongs",
    "refreshPartners",
    "searchUsers",
    "loadMoreUsers",
    "addPartner",
    "removePartner",
    "invitePartner",
    "refreshInvites",
    "respondInvite",
    "accept",
    "refreshPlaylists",
    "createPlaylist",
    "updatePlaylist",
    "playlistId",
    "description",
    "coverUrl",
    "server",
    "addSongsToPlaylist",
    "addSongToPlaylist",
    "refreshServerPlaylists",
    "createServerPlaylist",
    "addSongToServerPlaylist",
    "deletePlaylist",
    "removeSongFromPlaylist",
    "removeSongsFromPlaylist",
    "reorderPlaylist",
    "loadLyrics",
    "songForId",
    "uploadSong",
    "albumId",
    "newAlbumName",
    "lyrics",
    "updateSong",
    "albumName",
    "updateSongWithLyrics",
    "lyricChanges",
    "deleteSong",
    "deleteSongs",
    "guessTrackMetadata",
    "refreshAdminUsers",
    "loadMoreAdminUsers",
    "updatePermissions",
    "upload",
    "serverPlaylist",
    "updateRole",
    "role",
    "resetPassword",
    "kickUser",
    "authenticated",
    "autoLogin",
    "serverUrl",
    "darkMode",
    "errorMessage",
    "songs",
    "albums",
    "albumMatches",
    "transferTasks",
    "uploadedSongs",
    "hasMoreUploadedSongs",
    "downloadDirectory",
    "lineLyricsDirectory",
    "karaokeLyricsDirectory",
    "lyricsDisplayMode",
    "cacheEnabled",
    "cacheDirectory",
    "cacheUsageBytes",
    "transferManager",
    "TransferManager*",
    "hasMoreSongs",
    "songsLoading",
    "minimizeToTrayOnClose",
    "savePlaybackQueue",
    "rooms",
    "hasMoreRooms",
    "partners",
    "userResults",
    "hasMoreUsers",
    "playlists",
    "serverPlaylists",
    "invites",
    "roomState",
    "localQueue",
    "favoriteSongIds",
    "favoriteBusy",
    "playlistCreationBusy",
    "canUpload",
    "canManageServerPlaylist",
    "admin",
    "adminUsers",
    "hasMoreAdminUsers",
    "canControl",
    "roomOwner",
    "roomDeletionBusy",
    "batchDeletionBusy",
    "covers",
    "unreadInvites",
    "player",
    "AudioPlayer*",
    "coverCompletion",
    "CoverCompletionController*",
    "lyricsCompletion",
    "LyricsCompletionController*",
    "playbackMode"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13AppControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
     119,   14, // methods
      53, 1095, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      33,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  728,    2, 0x06,   54 /* Public */,
       3,    0,  729,    2, 0x06,   55 /* Public */,
       4,    0,  730,    2, 0x06,   56 /* Public */,
       5,    0,  731,    2, 0x06,   57 /* Public */,
       6,    0,  732,    2, 0x06,   58 /* Public */,
       7,    0,  733,    2, 0x06,   59 /* Public */,
       8,    0,  734,    2, 0x06,   60 /* Public */,
       9,    0,  735,    2, 0x06,   61 /* Public */,
      10,    0,  736,    2, 0x06,   62 /* Public */,
      11,    0,  737,    2, 0x06,   63 /* Public */,
      12,    0,  738,    2, 0x06,   64 /* Public */,
      13,    0,  739,    2, 0x06,   65 /* Public */,
      14,    0,  740,    2, 0x06,   66 /* Public */,
      15,    0,  741,    2, 0x06,   67 /* Public */,
      16,    0,  742,    2, 0x06,   68 /* Public */,
      17,    0,  743,    2, 0x06,   69 /* Public */,
      18,    0,  744,    2, 0x06,   70 /* Public */,
      19,    0,  745,    2, 0x06,   71 /* Public */,
      20,    0,  746,    2, 0x06,   72 /* Public */,
      21,    0,  747,    2, 0x06,   73 /* Public */,
      22,    0,  748,    2, 0x06,   74 /* Public */,
      23,    0,  749,    2, 0x06,   75 /* Public */,
      24,    0,  750,    2, 0x06,   76 /* Public */,
      25,    0,  751,    2, 0x06,   77 /* Public */,
      26,    0,  752,    2, 0x06,   78 /* Public */,
      27,    2,  753,    2, 0x06,   79 /* Public */,
      30,    0,  758,    2, 0x06,   82 /* Public */,
      31,    1,  759,    2, 0x06,   83 /* Public */,
      32,    0,  762,    2, 0x06,   85 /* Public */,
      33,    0,  763,    2, 0x06,   86 /* Public */,
      34,    2,  764,    2, 0x06,   87 /* Public */,
      38,    0,  769,    2, 0x06,   90 /* Public */,
      39,    2,  770,    2, 0x06,   91 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      42,    2,  775,    2, 0x02,   94 /* Public */,
      45,    3,  780,    2, 0x02,   97 /* Public */,
      47,    0,  787,    2, 0x02,  101 /* Public */,
      48,    0,  788,    2, 0x02,  102 /* Public */,
      49,    0,  789,    2, 0x02,  103 /* Public */,
      50,    1,  790,    2, 0x02,  104 /* Public */,
      52,    0,  793,    2, 0x02,  106 /* Public */,
      53,    1,  794,    2, 0x02,  107 /* Public */,
      53,    0,  797,    2, 0x22,  109 /* Public | MethodCloned */,
      54,    0,  798,    2, 0x02,  110 /* Public */,
      55,    0,  799,    2, 0x02,  111 /* Public */,
      56,    1,  800,    2, 0x02,  112 /* Public */,
      57,    1,  803,    2, 0x02,  114 /* Public */,
      59,    1,  806,    2, 0x02,  116 /* Public */,
      61,    0,  809,    2, 0x02,  118 /* Public */,
      62,    2,  810,    2, 0x02,  119 /* Public */,
      65,    2,  815,    2, 0x02,  122 /* Public */,
      67,    1,  820,    2, 0x02,  125 /* Public */,
      68,    1,  823,    2, 0x102,  127 /* Public | MethodIsConst  */,
      70,    1,  826,    2, 0x102,  129 /* Public | MethodIsConst  */,
      72,    1,  829,    2, 0x102,  131 /* Public | MethodIsConst  */,
      74,    1,  832,    2, 0x102,  133 /* Public | MethodIsConst  */,
      75,    0,  835,    2, 0x02,  135 /* Public */,
      76,    0,  836,    2, 0x02,  136 /* Public */,
      77,    1,  837,    2, 0x02,  137 /* Public */,
      78,    1,  840,    2, 0x02,  139 /* Public */,
      80,    0,  843,    2, 0x02,  141 /* Public */,
      81,    0,  844,    2, 0x02,  142 /* Public */,
      82,    1,  845,    2, 0x102,  143 /* Public | MethodIsConst  */,
      84,    1,  848,    2, 0x02,  145 /* Public */,
      85,    0,  851,    2, 0x02,  147 /* Public */,
      86,    2,  852,    2, 0x02,  148 /* Public */,
      89,    1,  857,    2, 0x02,  151 /* Public */,
      90,    1,  860,    2, 0x02,  153 /* Public */,
      92,    0,  863,    2, 0x02,  155 /* Public */,
      93,    1,  864,    2, 0x02,  156 /* Public */,
      95,    0,  867,    2, 0x02,  158 /* Public */,
      96,    1,  868,    2, 0x02,  159 /* Public */,
      98,    2,  871,    2, 0x02,  161 /* Public */,
     100,    0,  876,    2, 0x02,  164 /* Public */,
     101,    1,  877,    2, 0x02,  165 /* Public */,
     102,    1,  880,    2, 0x02,  167 /* Public */,
     103,    1,  883,    2, 0x02,  169 /* Public */,
     104,    1,  886,    2, 0x02,  171 /* Public */,
     105,    1,  889,    2, 0x02,  173 /* Public */,
     106,    1,  892,    2, 0x02,  175 /* Public */,
     107,    1,  895,    2, 0x02,  177 /* Public */,
     108,    2,  898,    2, 0x02,  179 /* Public */,
     109,    2,  903,    2, 0x02,  182 /* Public */,
     110,    2,  908,    2, 0x02,  185 /* Public */,
     113,    1,  913,    2, 0x02,  188 /* Public */,
     114,    1,  916,    2, 0x02,  190 /* Public */,
     115,    0,  919,    2, 0x02,  192 /* Public */,
     116,    1,  920,    2, 0x02,  193 /* Public */,
     117,    0,  923,    2, 0x02,  195 /* Public */,
     118,    1,  924,    2, 0x02,  196 /* Public */,
     119,    1,  927,    2, 0x02,  198 /* Public */,
     120,    1,  930,    2, 0x02,  200 /* Public */,
     121,    0,  933,    2, 0x02,  202 /* Public */,
     122,    2,  934,    2, 0x02,  203 /* Public */,
     124,    0,  939,    2, 0x02,  206 /* Public */,
     125,    1,  940,    2, 0x02,  207 /* Public */,
     126,    5,  943,    2, 0x02,  209 /* Public */,
     131,    3,  954,    2, 0x02,  215 /* Public */,
     132,    2,  961,    2, 0x02,  219 /* Public */,
     133,    0,  966,    2, 0x02,  222 /* Public */,
     134,    1,  967,    2, 0x02,  223 /* Public */,
     135,    2,  970,    2, 0x02,  225 /* Public */,
     136,    2,  975,    2, 0x02,  228 /* Public */,
     137,    3,  980,    2, 0x02,  231 /* Public */,
     138,    3,  987,    2, 0x02,  235 /* Public */,
     139,    3,  994,    2, 0x02,  239 /* Public */,
     140,    1, 1001,    2, 0x02,  243 /* Public */,
     141,    1, 1004,    2, 0x102,  245 /* Public | MethodIsConst  */,
     142,    8, 1007,    2, 0x02,  247 /* Public */,
     146,   10, 1024,    2, 0x02,  256 /* Public */,
     148,    9, 1045,    2, 0x02,  267 /* Public */,
     150,    1, 1064,    2, 0x02,  277 /* Public */,
     151,    1, 1067,    2, 0x02,  279 /* Public */,
     152,    1, 1070,    2, 0x102,  281 /* Public | MethodIsConst  */,
     153,    0, 1073,    2, 0x02,  283 /* Public */,
     154,    0, 1074,    2, 0x02,  284 /* Public */,
     155,    3, 1075,    2, 0x02,  285 /* Public */,
     158,    2, 1082,    2, 0x02,  289 /* Public */,
     160,    2, 1087,    2, 0x02,  292 /* Public */,
     161,    1, 1092,    2, 0x02,  295 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   28,   29,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   29,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 36,   35,   37,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Bool,   40,   41,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   43,   44,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   43,   46,   44,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   51,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   51,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QString, 0x80000000 | 36,   37,
    QMetaType::Void, 0x80000000 | 36,   58,
    QMetaType::Void, QMetaType::QString,   60,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   63,   64,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   66,   35,
    0x80000000 | 36, QMetaType::QString,   66,
    QMetaType::QString, QMetaType::QString,   69,
    QMetaType::QString, QMetaType::QString,   71,
    0x80000000 | 73, QMetaType::QString,   71,
    QMetaType::Int, QMetaType::QString,   71,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   63,
    QMetaType::Void, QMetaType::LongLong,   79,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::LongLong,   83,
    QMetaType::Void, QMetaType::LongLong,   83,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Bool,   87,   88,
    QMetaType::Void, 0x80000000 | 36,   58,
    QMetaType::Void, 0x80000000 | 73,   91,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,   94,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   97,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,   40,   99,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,   40,
    QMetaType::Void, QMetaType::LongLong,   40,
    QMetaType::Void, QMetaType::LongLong,   40,
    QMetaType::Void, 0x80000000 | 73,   91,
    QMetaType::Void, 0x80000000 | 73,   91,
    QMetaType::Void, 0x80000000 | 36,   58,
    QMetaType::Void, QMetaType::LongLong,   40,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   63,   40,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 73,   63,   91,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,  111,  112,
    QMetaType::Void, QMetaType::Int,  111,
    QMetaType::Void, 0x80000000 | 73,   91,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   51,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,   79,
    QMetaType::Void, QMetaType::LongLong,   79,
    QMetaType::Void, QMetaType::LongLong,   79,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   79,  123,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   63,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::Bool,  127,   63,  128,  129,  130,
    QMetaType::Void, QMetaType::LongLong, 0x80000000 | 73, QMetaType::Bool,  127,   91,  130,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,  127,   40,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   63,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,  127,   40,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Bool,  127,  130,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong, QMetaType::Bool,  127,   40,  130,
    QMetaType::Void, QMetaType::LongLong, 0x80000000 | 73, QMetaType::Bool,  127,   91,  130,
    QMetaType::Void, QMetaType::LongLong, 0x80000000 | 73, QMetaType::Bool,  127,   91,  130,
    QMetaType::Void, QMetaType::LongLong,   40,
    0x80000000 | 36, QMetaType::LongLong,   40,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString,   66,   28,   64,  143,  144,  145,   69,  129,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString,   79,   66,   28,   64,  143,  147,  144,  145,   69,  129,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, 0x80000000 | 36, QMetaType::QString,   79,   66,   28,   64,  143,  147,  144,  149,  129,
    QMetaType::Void, QMetaType::LongLong,   79,
    QMetaType::Void, 0x80000000 | 73,   91,
    0x80000000 | 73, QMetaType::QString,   66,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Bool, QMetaType::Bool,   79,  156,  157,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,   79,  159,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,   79,   44,
    QMetaType::Void, QMetaType::LongLong,   79,

 // properties: name, type, flags, notifyId, revision
     162, QMetaType::Bool, 0x00015001, uint(1), 0,
     163, QMetaType::Bool, 0x00015103, uint(2), 0,
      43, QMetaType::QString, 0x00015001, uint(1), 0,
     164, QMetaType::QString, 0x00015103, uint(3), 0,
     165, QMetaType::Bool, 0x00015103, uint(4), 0,
     166, QMetaType::QString, 0x00015001, uint(5), 0,
     167, 0x80000000 | 73, 0x00015009, uint(6), 0,
     168, 0x80000000 | 73, 0x00015009, uint(7), 0,
     169, 0x80000000 | 73, 0x00015009, uint(8), 0,
     170, 0x80000000 | 73, 0x00015009, uint(9), 0,
     171, 0x80000000 | 73, 0x00015009, uint(10), 0,
     172, QMetaType::Bool, 0x00015001, uint(10), 0,
     173, QMetaType::QString, 0x00015103, uint(11), 0,
     174, QMetaType::QString, 0x00015103, uint(11), 0,
     175, QMetaType::QString, 0x00015103, uint(11), 0,
     176, QMetaType::QString, 0x00015103, uint(11), 0,
     177, QMetaType::Bool, 0x00015103, uint(11), 0,
     178, QMetaType::QString, 0x00015103, uint(11), 0,
     179, QMetaType::LongLong, 0x00015001, uint(11), 0,
     180, 0x80000000 | 181, 0x00015409, uint(-1), 0,
     182, QMetaType::Bool, 0x00015001, uint(6), 0,
     183, QMetaType::Bool, 0x00015001, uint(6), 0,
     184, QMetaType::Bool, 0x00015103, uint(11), 0,
     185, QMetaType::Bool, 0x00015103, uint(11), 0,
     186, 0x80000000 | 73, 0x00015009, uint(12), 0,
     187, QMetaType::Bool, 0x00015001, uint(12), 0,
     188, 0x80000000 | 73, 0x00015009, uint(13), 0,
     189, 0x80000000 | 73, 0x00015009, uint(14), 0,
     190, QMetaType::Bool, 0x00015001, uint(14), 0,
     191, 0x80000000 | 73, 0x00015009, uint(15), 0,
     192, 0x80000000 | 73, 0x00015009, uint(16), 0,
     193, 0x80000000 | 73, 0x00015009, uint(17), 0,
     194, 0x80000000 | 36, 0x00015009, uint(18), 0,
     195, 0x80000000 | 73, 0x00015009, uint(20), 0,
     196, 0x80000000 | 73, 0x00015009, uint(15), 0,
     197, QMetaType::Bool, 0x00015001, uint(15), 0,
     198, QMetaType::Bool, 0x00015001, uint(31), 0,
     145, 0x80000000 | 36, 0x00015009, uint(21), 0,
     199, QMetaType::Bool, 0x00015001, uint(1), 0,
     200, QMetaType::Bool, 0x00015001, uint(1), 0,
     201, QMetaType::Bool, 0x00015001, uint(1), 0,
     202, 0x80000000 | 73, 0x00015009, uint(24), 0,
     203, QMetaType::Bool, 0x00015001, uint(24), 0,
     204, QMetaType::Bool, 0x00015001, uint(18), 0,
     205, QMetaType::Bool, 0x00015001, uint(18), 0,
     206, QMetaType::Bool, 0x00015001, uint(22), 0,
     207, QMetaType::Bool, 0x00015001, uint(6), 0,
     208, 0x80000000 | 36, 0x00015009, uint(23), 0,
     209, QMetaType::Int, 0x00015001, uint(17), 0,
     210, 0x80000000 | 211, 0x00015409, uint(-1), 0,
     212, 0x80000000 | 213, 0x00015409, uint(-1), 0,
     214, 0x80000000 | 215, 0x00015409, uint(-1), 0,
     216, QMetaType::QString, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject AppController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN13AppControllerE.offsetsAndSizes,
    qt_meta_data_ZN13AppControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13AppControllerE_t,
        // property 'authenticated'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'autoLogin'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'username'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'serverUrl'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'darkMode'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'errorMessage'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'songs'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'albums'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'albumMatches'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'transferTasks'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'uploadedSongs'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'hasMoreUploadedSongs'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'downloadDirectory'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'lineLyricsDirectory'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'karaokeLyricsDirectory'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'lyricsDisplayMode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'cacheEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'cacheDirectory'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'cacheUsageBytes'
        QtPrivate::TypeAndForceComplete<qint64, std::true_type>,
        // property 'transferManager'
        QtPrivate::TypeAndForceComplete<TransferManager*, std::true_type>,
        // property 'hasMoreSongs'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'songsLoading'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'minimizeToTrayOnClose'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'savePlaybackQueue'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'rooms'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'hasMoreRooms'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'partners'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'userResults'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'hasMoreUsers'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'playlists'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'serverPlaylists'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'invites'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'roomState'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'localQueue'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'favoriteSongIds'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'favoriteBusy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'playlistCreationBusy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'lyrics'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'canUpload'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'canManageServerPlaylist'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'admin'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'adminUsers'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'hasMoreAdminUsers'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'canControl'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'roomOwner'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'roomDeletionBusy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'batchDeletionBusy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'covers'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::true_type>,
        // property 'unreadInvites'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'player'
        QtPrivate::TypeAndForceComplete<AudioPlayer*, std::true_type>,
        // property 'coverCompletion'
        QtPrivate::TypeAndForceComplete<CoverCompletionController*, std::true_type>,
        // property 'lyricsCompletion'
        QtPrivate::TypeAndForceComplete<LyricsCompletionController*, std::true_type>,
        // property 'playbackMode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AppController, std::true_type>,
        // method 'playbackModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sessionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'autoLoginChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'serverUrlChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'darkModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'errorMessageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'songsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'albumsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'albumMatchesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'transferTasksChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'uploadedSongsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'settingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'roomsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'partnersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'userResultsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'playlistsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'serverPlaylistsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'invitesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'roomStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'roomLeaveForCloseFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'localQueueChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lyricsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'roomDeletionBusyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'coversChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'adminUsersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'invitationArrived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'openMessagesRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'infoMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'songSaved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'songDeleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'audioMetadataReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'playlistCreationBusyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'playlistCreationFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'login'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'registerAccount'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'logout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'loadMoreSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshAlbums'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refreshAlbums'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshUploadedSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadMoreUploadedSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enqueueUpload'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'enqueueDownload'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'openLocalFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clearSongCache'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchAlbumMatches'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'inspectAudio'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'findLyricsForAudio'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'readLyricsFile'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'classifyLyrics'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'parseLyrics'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'countLikelyCreditLines'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refreshRooms'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadMoreRooms'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'joinRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'leaveRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'leaveRoomBeforeClose'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'canDeleteRoom'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'deleteRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'refreshRoomState'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setController'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'playSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'playSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'togglePlayback'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'seek'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'nextSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setPlaybackMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'moveQueueBefore'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'previousSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addToQueue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'addToLocalQueue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'addToRoomQueue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'addSongsToQueue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'addSongsToLocalQueue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'playNext'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'toggleFavorite'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'createPlaylistWithSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'createPlaylistWithSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'moveQueueSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'removeQueueSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'removeQueueSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'refreshPartners'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchUsers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'loadMoreUsers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addPartner'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'removePartner'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'invitePartner'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'refreshInvites'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'respondInvite'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'refreshPlaylists'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updatePlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'addSongsToPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'addSongToPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'refreshServerPlaylists'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createServerPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addSongToServerPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'deletePlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'removeSongFromPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'removeSongsFromPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'reorderPlaylist'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'loadLyrics'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'songForId'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'uploadSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateSongWithLyrics'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'deleteSong'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'deleteSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'guessTrackMetadata'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refreshAdminUsers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadMoreAdminUsers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updatePermissions'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'updateRole'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetPassword'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'kickUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>
    >,
    nullptr
} };

void AppController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AppController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->playbackModeChanged(); break;
        case 1: _t->sessionChanged(); break;
        case 2: _t->autoLoginChanged(); break;
        case 3: _t->serverUrlChanged(); break;
        case 4: _t->darkModeChanged(); break;
        case 5: _t->errorMessageChanged(); break;
        case 6: _t->songsChanged(); break;
        case 7: _t->albumsChanged(); break;
        case 8: _t->albumMatchesChanged(); break;
        case 9: _t->transferTasksChanged(); break;
        case 10: _t->uploadedSongsChanged(); break;
        case 11: _t->settingsChanged(); break;
        case 12: _t->roomsChanged(); break;
        case 13: _t->partnersChanged(); break;
        case 14: _t->userResultsChanged(); break;
        case 15: _t->playlistsChanged(); break;
        case 16: _t->serverPlaylistsChanged(); break;
        case 17: _t->invitesChanged(); break;
        case 18: _t->roomStateChanged(); break;
        case 19: _t->roomLeaveForCloseFinished(); break;
        case 20: _t->localQueueChanged(); break;
        case 21: _t->lyricsChanged(); break;
        case 22: _t->roomDeletionBusyChanged(); break;
        case 23: _t->coversChanged(); break;
        case 24: _t->adminUsersChanged(); break;
        case 25: _t->invitationArrived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 26: _t->openMessagesRequested(); break;
        case 27: _t->infoMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 28: _t->songSaved(); break;
        case 29: _t->songDeleted(); break;
        case 30: _t->audioMetadataReady((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[2]))); break;
        case 31: _t->playlistCreationBusyChanged(); break;
        case 32: _t->playlistCreationFinished((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 33: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 34: _t->registerAccount((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 35: _t->logout(); break;
        case 36: _t->clearError(); break;
        case 37: _t->refreshAll(); break;
        case 38: _t->searchSongs((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 39: _t->loadMoreSongs(); break;
        case 40: _t->refreshAlbums((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 41: _t->refreshAlbums(); break;
        case 42: _t->refreshUploadedSongs(); break;
        case 43: _t->loadMoreUploadedSongs(); break;
        case 44: { QString _r = _t->enqueueUpload((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 45: _t->enqueueDownload((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 46: _t->openLocalFolder((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 47: _t->clearSongCache(); break;
        case 48: _t->searchAlbumMatches((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 49: _t->inspectAudio((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 50: { QVariantMap _r = _t->findLyricsForAudio((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 51: { QString _r = _t->readLyricsFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 52: { QString _r = _t->classifyLyrics((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 53: { QVariantList _r = _t->parseLyrics((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 54: { int _r = _t->countLikelyCreditLines((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 55: _t->refreshRooms(); break;
        case 56: _t->loadMoreRooms(); break;
        case 57: _t->createRoom((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 58: _t->joinRoom((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 59: _t->leaveRoom(); break;
        case 60: _t->leaveRoomBeforeClose(); break;
        case 61: { bool _r = _t->canDeleteRoom((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 62: _t->deleteRoom((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 63: _t->refreshRoomState(); break;
        case 64: _t->setController((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 65: _t->playSong((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 66: _t->playSongs((*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 67: _t->togglePlayback(); break;
        case 68: _t->seek((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 69: _t->nextSong(); break;
        case 70: _t->setPlaybackMode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 71: _t->moveQueueBefore((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 72: _t->previousSong(); break;
        case 73: _t->addToQueue((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 74: _t->addToLocalQueue((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 75: _t->addToRoomQueue((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 76: _t->addSongsToQueue((*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 77: _t->addSongsToLocalQueue((*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 78: _t->playNext((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 79: _t->toggleFavorite((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 80: _t->createPlaylistWithSong((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 81: _t->createPlaylistWithSongs((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[2]))); break;
        case 82: _t->moveQueueSong((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 83: _t->removeQueueSong((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 84: _t->removeQueueSongs((*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 85: _t->refreshPartners(); break;
        case 86: _t->searchUsers((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 87: _t->loadMoreUsers(); break;
        case 88: _t->addPartner((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 89: _t->removePartner((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 90: _t->invitePartner((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 91: _t->refreshInvites(); break;
        case 92: _t->respondInvite((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 93: _t->refreshPlaylists(); break;
        case 94: _t->createPlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 95: _t->updatePlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[5]))); break;
        case 96: _t->addSongsToPlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 97: _t->addSongToPlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 98: _t->refreshServerPlaylists(); break;
        case 99: _t->createServerPlaylist((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 100: _t->addSongToServerPlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 101: _t->deletePlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 102: _t->removeSongFromPlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 103: _t->removeSongsFromPlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 104: _t->reorderPlaylist((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 105: _t->loadLyrics((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 106: { QVariantMap _r = _t->songForId((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 107: _t->uploadSong((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[7])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[8]))); break;
        case 108: _t->updateSong((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[7])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[8])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[9])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[10]))); break;
        case 109: _t->updateSongWithLyrics((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[7])),(*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[8])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[9]))); break;
        case 110: _t->deleteSong((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 111: _t->deleteSongs((*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 112: { QVariantList _r = _t->guessTrackMetadata((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 113: _t->refreshAdminUsers(); break;
        case 114: _t->loadMoreAdminUsers(); break;
        case 115: _t->updatePermissions((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 116: _t->updateRole((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 117: _t->resetPassword((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 118: _t->kickUser((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::playbackModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::sessionChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::autoLoginChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::serverUrlChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::darkModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::errorMessageChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::songsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::albumsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::albumMatchesChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::transferTasksChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::uploadedSongsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::settingsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::roomsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::partnersChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::userResultsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::playlistsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::serverPlaylistsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::invitesChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::roomStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::roomLeaveForCloseFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 19;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::localQueueChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 20;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::lyricsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 21;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::roomDeletionBusyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 22;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::coversChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 23;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::adminUsersChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 24;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)(const QString & , const QString & );
            if (_q_method_type _q_method = &AppController::invitationArrived; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 25;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::openMessagesRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 26;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)(const QString & );
            if (_q_method_type _q_method = &AppController::infoMessage; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 27;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::songSaved; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 28;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::songDeleted; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 29;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)(const QString & , const QVariantMap & );
            if (_q_method_type _q_method = &AppController::audioMetadataReady; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 30;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::playlistCreationBusyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 31;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)(qint64 , bool );
            if (_q_method_type _q_method = &AppController::playlistCreationFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 32;
                return;
            }
        }
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 49:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< AudioPlayer* >(); break;
        case 50:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CoverCompletionController* >(); break;
        case 51:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< LyricsCompletionController* >(); break;
        case 19:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< TransferManager* >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->authenticated(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->autoLogin(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->username(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->serverUrl(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->darkMode(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->errorMessage(); break;
        case 6: *reinterpret_cast< QVariantList*>(_v) = _t->songs(); break;
        case 7: *reinterpret_cast< QVariantList*>(_v) = _t->albums(); break;
        case 8: *reinterpret_cast< QVariantList*>(_v) = _t->albumMatches(); break;
        case 9: *reinterpret_cast< QVariantList*>(_v) = _t->transferTasks(); break;
        case 10: *reinterpret_cast< QVariantList*>(_v) = _t->uploadedSongs(); break;
        case 11: *reinterpret_cast< bool*>(_v) = _t->hasMoreUploadedSongs(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->downloadDirectory(); break;
        case 13: *reinterpret_cast< QString*>(_v) = _t->lineLyricsDirectory(); break;
        case 14: *reinterpret_cast< QString*>(_v) = _t->karaokeLyricsDirectory(); break;
        case 15: *reinterpret_cast< QString*>(_v) = _t->lyricsDisplayMode(); break;
        case 16: *reinterpret_cast< bool*>(_v) = _t->cacheEnabled(); break;
        case 17: *reinterpret_cast< QString*>(_v) = _t->cacheDirectory(); break;
        case 18: *reinterpret_cast< qint64*>(_v) = _t->cacheUsageBytes(); break;
        case 19: *reinterpret_cast< TransferManager**>(_v) = _t->transferManager(); break;
        case 20: *reinterpret_cast< bool*>(_v) = _t->hasMoreSongs(); break;
        case 21: *reinterpret_cast< bool*>(_v) = _t->songsLoading(); break;
        case 22: *reinterpret_cast< bool*>(_v) = _t->minimizeToTrayOnClose(); break;
        case 23: *reinterpret_cast< bool*>(_v) = _t->savePlaybackQueue(); break;
        case 24: *reinterpret_cast< QVariantList*>(_v) = _t->rooms(); break;
        case 25: *reinterpret_cast< bool*>(_v) = _t->hasMoreRooms(); break;
        case 26: *reinterpret_cast< QVariantList*>(_v) = _t->partners(); break;
        case 27: *reinterpret_cast< QVariantList*>(_v) = _t->userResults(); break;
        case 28: *reinterpret_cast< bool*>(_v) = _t->hasMoreUsers(); break;
        case 29: *reinterpret_cast< QVariantList*>(_v) = _t->playlists(); break;
        case 30: *reinterpret_cast< QVariantList*>(_v) = _t->serverPlaylists(); break;
        case 31: *reinterpret_cast< QVariantList*>(_v) = _t->invites(); break;
        case 32: *reinterpret_cast< QVariantMap*>(_v) = _t->roomState(); break;
        case 33: *reinterpret_cast< QVariantList*>(_v) = _t->localQueue(); break;
        case 34: *reinterpret_cast< QVariantList*>(_v) = _t->favoriteSongIds(); break;
        case 35: *reinterpret_cast< bool*>(_v) = _t->favoriteBusy(); break;
        case 36: *reinterpret_cast< bool*>(_v) = _t->playlistCreationBusy(); break;
        case 37: *reinterpret_cast< QVariantMap*>(_v) = _t->lyrics(); break;
        case 38: *reinterpret_cast< bool*>(_v) = _t->canUpload(); break;
        case 39: *reinterpret_cast< bool*>(_v) = _t->canManageServerPlaylist(); break;
        case 40: *reinterpret_cast< bool*>(_v) = _t->admin(); break;
        case 41: *reinterpret_cast< QVariantList*>(_v) = _t->adminUsers(); break;
        case 42: *reinterpret_cast< bool*>(_v) = _t->hasMoreAdminUsers(); break;
        case 43: *reinterpret_cast< bool*>(_v) = _t->canControl(); break;
        case 44: *reinterpret_cast< bool*>(_v) = _t->roomOwner(); break;
        case 45: *reinterpret_cast< bool*>(_v) = _t->roomDeletionBusy(); break;
        case 46: *reinterpret_cast< bool*>(_v) = _t->batchDeletionBusy(); break;
        case 47: *reinterpret_cast< QVariantMap*>(_v) = _t->covers(); break;
        case 48: *reinterpret_cast< int*>(_v) = _t->unreadInvites(); break;
        case 49: *reinterpret_cast< AudioPlayer**>(_v) = _t->player(); break;
        case 50: *reinterpret_cast< CoverCompletionController**>(_v) = _t->coverCompletion(); break;
        case 51: *reinterpret_cast< LyricsCompletionController**>(_v) = _t->lyricsCompletion(); break;
        case 52: *reinterpret_cast< QString*>(_v) = _t->playbackMode(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 1: _t->setAutoLogin(*reinterpret_cast< bool*>(_v)); break;
        case 3: _t->setServerUrl(*reinterpret_cast< QString*>(_v)); break;
        case 4: _t->setDarkMode(*reinterpret_cast< bool*>(_v)); break;
        case 12: _t->setDownloadDirectory(*reinterpret_cast< QString*>(_v)); break;
        case 13: _t->setLineLyricsDirectory(*reinterpret_cast< QString*>(_v)); break;
        case 14: _t->setKaraokeLyricsDirectory(*reinterpret_cast< QString*>(_v)); break;
        case 15: _t->setLyricsDisplayMode(*reinterpret_cast< QString*>(_v)); break;
        case 16: _t->setCacheEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 17: _t->setCacheDirectory(*reinterpret_cast< QString*>(_v)); break;
        case 22: _t->setMinimizeToTrayOnClose(*reinterpret_cast< bool*>(_v)); break;
        case 23: _t->setSavePlaybackQueue(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *AppController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AppController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13AppControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AppController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 119)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 119;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 119)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 119;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 53;
    }
    return _id;
}

// SIGNAL 0
void AppController::playbackModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AppController::sessionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AppController::autoLoginChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AppController::serverUrlChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AppController::darkModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AppController::errorMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AppController::songsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void AppController::albumsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void AppController::albumMatchesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void AppController::transferTasksChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void AppController::uploadedSongsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void AppController::settingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void AppController::roomsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void AppController::partnersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void AppController::userResultsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void AppController::playlistsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void AppController::serverPlaylistsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void AppController::invitesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 17, nullptr);
}

// SIGNAL 18
void AppController::roomStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}

// SIGNAL 19
void AppController::roomLeaveForCloseFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 19, nullptr);
}

// SIGNAL 20
void AppController::localQueueChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}

// SIGNAL 21
void AppController::lyricsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 21, nullptr);
}

// SIGNAL 22
void AppController::roomDeletionBusyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 22, nullptr);
}

// SIGNAL 23
void AppController::coversChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 23, nullptr);
}

// SIGNAL 24
void AppController::adminUsersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void AppController::invitationArrived(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 25, _a);
}

// SIGNAL 26
void AppController::openMessagesRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}

// SIGNAL 27
void AppController::infoMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 27, _a);
}

// SIGNAL 28
void AppController::songSaved()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void AppController::songDeleted()
{
    QMetaObject::activate(this, &staticMetaObject, 29, nullptr);
}

// SIGNAL 30
void AppController::audioMetadataReady(const QString & _t1, const QVariantMap & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 30, _a);
}

// SIGNAL 31
void AppController::playlistCreationBusyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 31, nullptr);
}

// SIGNAL 32
void AppController::playlistCreationFinished(qint64 _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 32, _a);
}
QT_WARNING_POP
