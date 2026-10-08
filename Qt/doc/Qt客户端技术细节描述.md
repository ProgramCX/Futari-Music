# 同步听歌系统 —— Qt 6.8 / QML 客户端技术细节描述

## 歌单读取与房间解散补充（2026-10-09）

歌单列表与详情仅展示有效歌曲，失效关联不阻塞读取；排序以这些有效歌曲为全集。喜欢按钮写入服务器端「我喜欢」歌单，操作前刷新身份并复用已有歌单，异步列表刷新按请求代次防止旧结果覆盖新状态。

房间创建者通过 `DELETE /api/rooms/{id}` 解散房间，离开后仍有权限。界面使用统一圆角确认弹窗；`ROOM_DELETED.data={roomId}` 使成员退出共享播放并恢复个人播放快照为暂停状态，不清空个人队列、不退出账号。客户端以当前房间状态校验权限，公开列表用于离房后的创建者操作，最终权限由后端确认。

> Qt 6.8 + QML 客户端代码位于 [`../FutariMusic`](../FutariMusic)，实际已实现内容、运行方法和未完成项见 [`客户端实现说明.md`](客户端实现说明.md)。本文保留完整产品与技术目标；联调字段以 [`../../SpringBoot/doc/后端实现与部署.md`](../../SpringBoot/doc/后端实现与部署.md) 和现有代码为准。

当前已落地的界面约定：`Theme.qml` 按语义集中定义明暗背景、文字、图标、按钮、输入框、圆角与动效令牌；页面和控件复用这些值，不自行硬编码主题样式。`FutariButton.qml` / `FutariToolButton.qml` / `FutariTextField.qml` 提供统一圆角控件，hover/pressed/focus 使用克制的颜色变化和主题定义的短时反馈；`SvgIcon.qml` 从 `qml/assets/icons` 加载白色矢量蒙版 SVG，并统一映射到主题图标颜色。功能图标禁止使用 Unicode/ASCII 字符拼接。账户与退出登录位于独立 `AccountPage.qml`，通过主界面右上角账户按钮进入；该页显示账号与管理员身份，并提供浅色/深色开关。消息邀请仍由右上角消息按钮进入。首次注册逻辑由后端决定：空用户表的首个账户自动获得 `ADMIN`，其余为 `USER`。

界面布局参考用户提供的 QQ 音乐主窗口、播放队列、歌词页截图，以及 QQ 登录窗口截图：借鉴区域划分、信息密度、浅色内容页与深色登录卡片。客户端使用 Futari 自己的名称、图标和内容；截图中的 VIP、购买、扫码登录等功能不对应当前后端，界面位置改放房间、搭子、曲库和歌单。

本地参考图：[`主界面与队列`](../参考图片/01-主界面与队列.png)、[`歌词与队列`](../参考图片/02-歌词与队列.png)、[`登录窗口`](../参考图片/03-登录窗口.png)、[`沉浸歌词`](../参考图片/04-沉浸歌词.png)、[`主界面列表`](../参考图片/05-主界面列表.png)。这些图仅用于布局参考，不作为客户端品牌素材。浅色/深色主题由右上角手动切换，选择保存在 `QSettings`。

邀请到达时，客户端通过 `QSystemTrayIcon::showMessage` 发系统通知，同时更新右上角消息图标的数量；点击系统通知打开消息页，在消息页同意或拒绝。启动/重连后通过 `GET /api/invites` 恢复仍有效的待处理邀请。系统托盘不可用时，应用内提示和消息页仍可用。

## 1. 技术栈总览

| 模块 | 选型 | 说明 |
|---|---|---|
| Qt 版本 | **Qt 6.8**，CMake 构建 | Windows 桌面客户端，项目最低版本固定为 6.8 |
| UI | **Qt Quick / QML**：`QtQuick`、`QtQuick.Controls`、`QtQuick.Layouts`、`QtQuick.Window` | 页面、列表、弹层、窗口与动画均由 QML 实现，不使用 Widgets 页面 |
| QML 与 C++ | `QQmlApplicationEngine`、`qt_add_qml_module`、`QObject` / `QAbstractListModel` | C++ 提供网络、状态、缓存与模型，QML 负责显示和交互 |
| 播放 | `QMediaPlayer` + `QAudioOutput` | 支持 `setPosition()` 精确定位，满足秒级同步 |
| HTTP | `QNetworkAccessManager` | REST 调用 + Range 断点续传；边下边播仍为后续目标 |
| WebSocket | `QWebSocket` | 对接后端原生 WS |
| JSON | `QJsonDocument` / `QJsonObject` | 协议解析 |
| 任务与偏好 | `QSettings` | 按用户持久化传输任务、下载目录、缓存开关和缓存目录；不使用独立 SQLite 缓存索引 |
| 元信息 | `QMediaPlayer` / `QMediaMetaData` | 读取标签、时长及嵌入封面；解码能力依 Qt 媒体后端 |

CMake 用 `qt_standard_project_setup(REQUIRES 6.8)`、`qt_add_qml_module` 定义客户端 QML 模块，链接 `Quick`、`QuickControls2`、`Multimedia`、`Network`、`WebSockets`、`Widgets` 和 `Svg`。本工程不链接 Qt SQL。技术选型参考 [Qt 6.8 QML 模块的 CMake 文档](https://doc.qt.io/qt-6.8/qt-add-qml-module.html) 和 [Qt 6.8 Window 类型](https://doc.qt.io/qt-6.8/qml-qtquick-window.html)。

---

## 2. 客户端分层架构

```
UI 层（QML）
  ├── LoginWindow.qml / AppWindow.qml（独立登录窗与主窗口）
  ├── pages/LibraryPage.qml、RoomPage.qml、PartnerPage.qml、PlaylistPage.qml、AdminPage.qml
  ├── pages/LyricPage.qml（沉浸式歌词页）
  └── components/AppSidebar.qml、AppHeader.qml、SongRow.qml、PlayerBar.qml、QueueDrawer.qml、InviteToast.qml
        │
服务层（Service，QObject 单例）
  ├── AuthService        登录/登出、token 保管
  ├── UserService        搜索用户
  ├── PartnerService     搭子收藏/列表
  ├── AudioMetadataReader 读取标签与嵌入封面
  ├── TransferManager    上传/下载任务队列与任务持久化
  ├── SongService        曲库、含歌词与封面的上传、下载
  ├── PlaylistService    个人歌单 + 服务器歌单
  ├── RoomService        房间 CRUD、加入/退出
  └── InviteService      邀请发起/响应
        │
核心层（Core）
  ├── ApiClient          封装 QNAM：自动附带 JWT、统一错误处理
  ├── WsClient           QWebSocket 封装：自动重连、消息分发（信号槽）
  ├── ClockSync          ping/pong 估算 RTT 与服务器时间偏移
  ├── SyncController     根据广播指令 + 偏移量驱动播放器对齐
  ├── PlayerEngine       QMediaPlayer 封装：本地文件/网络流统一接口
  └── AudioPlayer        hash 本地缓存、LRU 淘汰、校验后播放
```

关键约定：**QML 不直接碰网络或数据库**。QML 绑定 C++ `QObject` 属性、调用明确的 `Q_INVOKABLE` 命令；列表使用 `QAbstractListModel` 的稳定角色。所有网络交互走服务层，核心层通过 Qt 信号把事件（收到 INVITE、播放指令、搭子上线）抛给 QML 更新。大列表使用 `ListView` 委托复用，避免一次创建所有歌曲行。

建议目录按 `src/core`、`src/services`、`src/models`、`qml/pages`、`qml/components`、`qml/theme` 划分。`qml/theme` 集中颜色、圆角、字体、间距与窗口尺寸；页面只使用这些共享值。C++ 把 `AuthService`、`PlayerEngine` 等状态对象注册到 QML，上层页面只调用公开命令，不在 QML 中拼接 JWT、Redis 键或 HTTP URL。

---

## 3. 核心层详细设计

### 3.1 ApiClient

- 持有 `QNetworkAccessManager`，每个请求自动加 `Authorization: Bearer {jwt}`；
- 统一返回 `QFuture` 或回调式 API；401 时统一跳转登录；
- 文件下载单独一个方法：支持 Range 头、进度信号、写临时文件完成后改名。

### 3.2 WsClient

- 连接：与 API 同源；HTTPS 对应 `wss://host/ws?token={jwt}`，本地 HTTP 开发环境对应 `ws://host/ws?token={jwt}`；
- **自动重连**：指数退避（1s/2s/4s…上限 30s），重连成功后自动重新进房并拉取房间全量状态恢复现场；
- 收到 JSON 后按 `type` 分发成 Qt 信号：`playReceived`、`pauseReceived`、`inviteReceived`、`partnerStatusChanged`、`kicked` 等。

### 3.3 ClockSync（时钟校准）

- 连接建立后连续发 5 次 PING（间隔 200ms），取最小 RTT 的那次估算偏移：
  `offset = serverTime − clientSendTime − RTT/2`；
- 之后每 60s 校准一次；
- 暴露接口：`qint64 toServerTime(localMs)` / `qint64 toLocalTime(serverMs)`。

### 3.4 SyncController（同步大脑，最重要）

- 收到 PLAY：目标位置 = `playback.positionMs + (nowLocal() 换算的服务器时间 − playback.serverTimestamp)`；这里取播放状态里的 `serverTimestamp`，消息信封的 `serverTime` 用于校时和接收时刻参考；
  → 调 CacheManager 取文件（见 3.5）→ 就绪后 `player->setPosition(目标位置)` 再 `play()`；
- 收到 PAUSE：先 `pause()`，再 `setPosition(positionMs)` 对齐；
- 收到 SEEK：`setPosition()`；
- 收到定期 SYNC：与本地 `player->position()` 比较，**偏差 >500ms 才纠正**，小偏差忽略（避免可感知的跳音）；
- 本机操作（房主/有控制权限者）：把操作反向上行——本地播放位置 + ClockSync 换算成服务器时间，发 PLAY/PAUSE/SEEK。

### 3.5 CacheManager（缓存机制）

- 缓存目录：`%AppData%/SyncMusic/music_cache/`，文件名 = `{hash}.{ext}`；
- SQLite 索引表：

```sql
CREATE TABLE cache_index (
    hash TEXT PRIMARY KEY,
    path TEXT NOT NULL,
    size INTEGER,
    last_used INTEGER          -- 时间戳，LRU 依据
);
```

- 播前流程：
  1. 查索引，命中 → 更新 `last_used`，直接给 PlayerEngine 本地路径，**零下载**；
  2. 未命中 → 走 HTTP Range 下载到 `.part` 文件，完成后校验 SHA-256、改名并开始播放；下载中断后保留有效部分供下次续传。音频经 Nginx `X-Accel-Redirect` 或直连 Spring 流式返回，两种路径均支持 Range；当前客户端需完整下载校验后才开播；
- **容量管理**：播放器按约 512 MiB 上限清理较旧缓存；设置页统计真实缓存文件大小并可清理应用生成的 hash 缓存文件。
- 下载中断后保留 `.part`，下次用 Range 从已下载字节继续；任务下载完成前写入 `.part` 并校验 SHA-256 后原子改名。上传队列并发为 1、下载队列并发为 2。下载暂停保留部分文件；上传请求暂停会终止请求，恢复时从头开始。

### 3.6 PlayerEngine

- 统一两个输入源：本地 `QUrl::fromLocalFile`（缓存命中）或渐进式缓冲（未命中）；
- 暴露：`play()` / `pause()` / `seekTo(ms)` / `positionChanged` 信号；
- 播放结束自动触发"下一首"逻辑（个人模式直接切；房间模式由控制端决定）。
- 下载、校验或解码失败时，C++ 播放器通过统一错误信号通知应用；主窗口以主题化提示展示错误，避免播放按钮无反馈。

### 3.7 歌曲信息、歌词与共享专辑

- 曲库和歌单中的歌曲摘要新增 `albumId`、`album`、`hasLyrics`、`coverUrl` 字段，原有 `title`、`artist`、`hash` 等字段保持不变。`albumId` 可为空，多首歌可关联同一个专辑；`coverUrl` 是相对 API 路径，可为空；`hasLyrics=false` 时不显示歌词入口。
- 单曲和批量上传均先预处理后由用户确认。`AudioMetadataReader` 使用 Qt Multimedia 异步读 Title、ContributingArtist/Author、AlbumTitle、AlbumArtist、Duration、封面图与文件扩展名；FLAC 另解析 Vorbis Comment、STREAMINFO 和 PICTURE block，弥补媒体后端漏报专辑标签、时长或封面的情况。标签读取不可用时标题回退到不含扩展名的文件名。文件名正则提供正序/倒序候选。对音频同目录按文件主体不区分大小写找同名 `.lrc`，其次 `.txt`；读取 UTF-8/UTF-16 BOM，失败时回退系统编码。Web 运行时不能访问任意用户目录，但本项目是桌面 Qt 客户端，使用文件对话框选定音频后扫描该音频的兄弟文件。
- 专辑身份按名称和专辑艺术家组合；多个同名候选不能唯一确定时不自动关联。批量列表支持逐首编辑、移除和经确认的歌手/专辑批量应用；新增专辑在同批次中按同一身份复用。上传提交 `albumId` 或 `newAlbumName/newAlbumArtist`，可分别提交共享 `albumCoverFile` 与单曲 `songCoverFile`；歌词提交 `lyricsText` 或 `lyricsFile`。重复音频由服务端 SHA-256 去重并保留已有元数据。
- 音频支持 mp3/flac/aac/ogg/wav/m4a；服务端单音频上限由 `FUTARI_UPLOAD_MAX_AUDIO_SIZE` 配置（默认 `64MB`），完整请求上限由 `FUTARI_UPLOAD_MAX_REQUEST_SIZE` 配置（默认 `80MB`）。图片仍最多 5 MiB，歌词文件仍最多 256 KiB。超限时客户端展示服务端返回的配置限制错误，不依赖固定容量文案。FLAC 标签和封面由客户端解析兜底；其他格式的元数据读取还受 Qt Multimedia 后端和解码插件支持情况影响。
- 管理员右键曲库歌曲行，从菜单进入 `SongEditPage.qml`。页面加载当前歌曲摘要与 `GET /api/songs/{id}/lyrics`，并预填歌名、歌手、专辑和歌词。保存通过 multipart `PUT /api/songs/{id}` 提交，可选替换音频、歌词文件和专辑图；`albumId=0` 清除专辑关联。编辑已有专辑时可改名或换图，UI 提示共享变更会影响同专辑的全部歌曲。红色删除按钮二次确认后调用管理员 `DELETE /api/songs/{id}`，后端软删除歌曲。
- 歌词通过 `GET /api/songs/{id}/lyrics` 获取 `{songId,lyrics}`；`lyrics` 可为空。若含 LRC 时间标签，客户端解析并随播放位置高亮；纯文本按行展示。服务端原样存储歌词。
- 封面通过 `GET /api/songs/{id}/cover` 获取图片字节。该请求必须带 `Authorization: Bearer ...`：由 C++ `AppController` 异步下载到封面缓存，再把本地 `file:` URL 暴露给 QML `Image.source`。直接把需鉴权的 `coverUrl` 交给 QML `Image` 会缺少认证头。无封面时显示默认占位图。歌曲编辑后客户端失效对应缓存；更换共享专辑图片时清理全部本地封面缓存，确保曲库、歌单和播放器刷新到新图。
- 服务端按音频 SHA-256 去重，重复上传同一音频会返回原歌曲资料，不覆盖原歌手、专辑、歌词或封面。上传成功后以响应中的歌曲元数据刷新列表，不把本次表单内容当成最终元数据。歌曲编辑替换音频时若 hash 已属于另一首歌曲会收到重复错误。

---

## 4. QML 界面规格

### 4.1 登录窗口（参考 QQ 登录图）

- 独立、居中的紧凑窗口，约 `440 × 640` 逻辑像素，深紫至蓝紫的柔和背景、圆角容器；顶部有 Futari 标识/用户头像占位和关闭按钮。若采用无系统标题栏，拖动用 Qt 6.8 `Window.startSystemMove()`，边缘缩放用 `startSystemResize()`；保留窗口键盘焦点、关闭与最小化行为。
- 从上到下依次为：头像区域、账号输入（可从本机记住的用户名中选择）、密码输入与显示切换、`记住账号`/`自动登录` 选项、显著的登录按钮、错误提示区、注册账号和服务器地址入口。键盘 Enter 提交，提交中禁用重复点击并显示进度；错误留在表单内，不用模态弹窗打断输入。
- `记住账号` 只保存用户名；`自动登录` 使用现有 JWT，在启动时请求受保护接口确认有效，401 清理令牌并回到登录窗。密码不保存到 `QSettings`。注册使用 `POST /api/auth/register`，登录使用 `POST /api/auth/login`；这两个操作都使用应用自己的账号，不出现 QQ 号、QQ 图标或扫码登录入口。
- 登录成功后关闭登录窗并打开主窗口，先建立 WebSocket、校准时钟再恢复房间。若服务地址未配置，先引导填写地址；连接失败在表单内给出可重试状态。

### 4.2 主窗口骨架（参考 QQ 音乐列表图）

```
┌──────────────────────────────┬───────────────────────────────────────────────┐
│ 左侧导航栏                    │ 顶部工具栏：返回/前进、搜索、当前房间、窗口控制 │
│ 用户头像 / 昵称 / 在线状态    ├───────────────────────────────────────────────┤
│ 四个快捷入口                  │ 主内容：标题、分类/操作区、歌曲或成员列表       │
│ 一起听房间 / 搭子 / 曲库       │                              ┌──────────────┤
│ 我的歌单                      │                              │ 播放队列抽屉 │
│ 服务器歌单                    │                              │              │
├──────────────────────────────┴──────────────────────────────┴──────────────┤
│ 固定底部播放条：封面与歌曲信息 │ 进度与播放控制 │ 音量、歌词、队列、房间状态 │
└───────────────────────────────────────────────────────────────────────────────┘
```

- 浅灰窗口底色，导航、内容区、队列为层次分明的浅色面板；深色文字，绿色只用于当前歌曲、选中项和主要播放动作。圆角、留白与行悬停状态参考截图的信息密度，但使用 Futari 自己的图标和文案。
- 常规宽度下左栏约 `280–320`、顶部约 `64–72`、底部播放条约 `96–108` 逻辑像素；中央区域随窗口伸缩。队列宽约 `380–460`，在窗口较窄时改为覆盖内容区的抽屉，避免把歌名挤得无法阅读。主窗口建议最小约 `1000 × 680`；再窄时左栏折叠为图标导航。尺寸按 Qt 高 DPI 逻辑像素处理。
- 左栏顶部是账号/在线状态和快捷入口；中段显示正在参与的房间、搭子、我的歌单、服务器歌单、本地下载；`ADMIN` 才显示管理入口。原图的会员、购买、试用和视频位置不保留为空壳，改为上述已有业务。首次打开默认显示曲库或上次访问的有效页面；不存在的歌单要回退曲库。
- 顶部搜索栏搜索歌曲名称、歌手、专辑；切换到搭子页时搜索昵称。列表页顶部显示页面标题、筛选/操作按钮；歌曲行包含封面、歌名、歌手、专辑、时长、操作菜单。鼠标悬停显示行内操作，当前播放行用绿色标识。`durationMs=null` 时显示 `--:--`，不凭空生成时长。
- 底部播放条始终固定：左侧封面、歌名/歌手与“加入我的歌单”按钮；中间上一首、播放/暂停、下一首、进度条与时间；右侧音量、歌词页、播放队列和当前房间状态。原图心形图标若使用，其动作明确为“加入歌单”，不暗示后端存在独立的喜欢歌曲接口。房间成员没有控制权时，控制按钮不可操作并说明原因，仍能看到共享进度。

### 4.3 播放队列与歌词页（参考播放队列、歌词和唱片图）

- 右侧 `QueueDrawer.qml` 由播放条按钮切换；列表为专辑封面 + 歌名 + 歌手，当前播放行高亮，顶部显示歌曲数及队列标题。房间模式展示服务器 `RoomStateResponse.songIds` 和后续 `PLAYLIST_UPDATE`；个人播放模式展示本地临时队列。无控制权的房间成员只能浏览，不能删除、拖动或切歌。
- `LyricPage.qml` 从播放条歌词按钮进入，底部播放条保持可用。队列打开时采用“左侧歌词 + 右侧队列”；队列关闭时采用“左侧歌词 + 右侧大封面/唱片视觉”。背景取当前封面的模糊色彩或默认渐变；歌曲名、歌手置于歌词上方，当前歌词行高亮，前后行降低对比，滚动到当前行但不在每帧重排。
- LRC 根据时间标签和播放器位置高亮、允许手动滚动后回到自动跟随；纯文本歌词按行显示，不伪造逐字同步。无歌词时显示歌曲信息与明确的“暂无歌词”。队列、歌词页和普通列表共享同一个 `PlayerEngine` 与播放状态，切页不重新创建播放器。

### 4.4 页面与业务对应

| QML 页面 | 主内容 | 关键行为 |
|---|---|---|
| `LibraryPage.qml` | 曲库搜索、封面歌曲列表、上传入口 | 无上传权限时置灰；管理员默认拥有上传权限。上传弹窗支持选已有专辑/新建专辑、歌词文本/文件、专辑图片和文件名正则识别候选；管理员右键歌曲可进入编辑页 |
| `SongEditPage.qml` | 管理员编辑歌曲和共享专辑 | 预填服务器歌曲与歌词；可改歌名、歌手、歌词、专辑关系、专辑名和专辑图，可替换音频；红色删除按钮带二次确认。修改已有关联专辑的名称/图片会影响专辑下所有歌曲 |
| `RoomPage.qml` | 房间标题、成员头像、共享歌曲列表和控制权标识 | 建房/进房/离房、邀请搭子；房主授予或撤销成员控制权；批量把房间歌曲加入个人歌单 |
| `PartnerPage.qml` | 昵称搜索、收藏的搭子、在线状态和所在房间 | 收藏/取消收藏，房间中发送邀请；邀请到达用右上角 `InviteToast.qml` 接受或拒绝 |
| `PlaylistPage.qml` | 个人或服务器歌单及歌曲列表 | 左侧分别提供“我的歌单”和“服务器歌单”入口；可创建、改名、删除歌单，批量从曲库加歌、移除歌曲并调整顺序。服务器歌单编辑仅管理员或有权限用户可用 |
| `AdminPage.qml` | 用户与权限列表 | 仅管理员显示；用主题化复选框显示并修改上传/服务器歌单权限、切换角色、重置密码和踢下线；管理员默认拥有上传和服务器歌单管理权限 |

`AppWindow.qml` 负责上述页面导航、抽屉开合和全局弹层，不让每个页面分别创建播放条或 WebSocket。`FutariCheckBox`、`FutariComboBox`、注册弹窗、上传弹窗和邀请提示共用 `Theme.qml` 的语义色、圆角与焦点状态，避免平台默认控件颜色破坏浅色/深色主题。

---

## 5. 关键交互流程

### 5.1 登录
`LoginWindow.qml` 输入账号密码 → `AuthService` 调 `POST /api/auth/login` → 成功后保存 JWT 与用户权限、打开 `AppWindow.qml` → 建立 WS 连接 → `ClockSync` 校准。启动时若启用自动登录且有 token，先请求受保护接口确认有效，成功才进入主窗口；失效则留在登录窗并保留已记住的用户名。登录窗外观参考 QQ 登录图，认证仍使用 Futari 后端。

### 5.2 收藏搭子 → 一键邀请
搜索昵称 → 收藏 → 搭子页出现该用户 → 自己在房间中时点击「邀请」→ 收到对方接受的通知后成员列表刷新（MEMBER_JOIN）。

### 5.3 接受邀请
InviteToast 弹出 → 点「接受」→ InviteService 调 accept → 拿到房间全量状态 → SyncController 按校准公式定位 → CacheManager 取歌 → 从当前进度无缝开播 → 切到房间页。

### 5.4 听歌中收藏歌曲
底部播放条点“加入歌单”图标 → 弹个人歌单列表（可现场新建）→ 选中即加入。若图标采用心形，悬停提示必须写“加入我的歌单”。

### 5.5 从一起听列表导入个人歌单
房间页播放列表勾选若干首（或全选）→ 「加入我的歌单」→ 选目标歌单 → 批量提交 songIds → 提示"成功加入 N 首（M 首已存在）"（服务端幂等去重）。

### 5.6 管理页面（admin）
admin 登录后左侧导航多出「管理」入口：用户列表 + 权限开关 + 角色切换 + 重置密码 + 强制下线。修改权限后可选择顺手踢下线，让对方重登生效。

### 5.7 服务器曲库与公共歌单
管理员或有上传权限的用户在左侧「曲库」打开「上传歌曲」，选音频后客户端对文件名做正则切分并预填歌名、歌手；歌名和歌手字段各自提供正序/倒序候选。用户可选已有专辑，也可即时创建专辑并上传专辑图；歌词可填文本或选择 `.lrc` / `.txt`。上传调用 `POST /api/songs/upload`。管理员可右键曲库歌曲进入 `SongEditPage.qml`，页面预填服务端歌曲名、歌手、专辑和歌词，可更换音频、歌词、专辑关系、专辑名和专辑图片。修改已选专辑名或图片会影响关联该专辑的所有歌曲，页面会提示；红色删除按钮确认后调用管理员软删除接口。所有人都可浏览服务器歌单；管理员或有 `canManageServerPlaylist` 权限的用户可从左侧「服务器歌单」进入管理，创建、改名、删除歌单，批量从曲库选歌、移除歌曲和调整顺序，对应 `/api/server-playlists` 接口。

---

## 6. 本地持久化

| 内容 | 位置 |
|---|---|
| JWT、服务器地址、窗口几何、记住的用户名、自动登录开关 | QSettings（注册表/ini）；不保存明文密码 |
| 当前下载目录 | QSettings；默认为系统 Music 目录 |
| 缓存开关与缓存目录 | QSettings；用户可设置页修改 |
| 下载/上传任务与历史 | QSettings，按用户 ID 隔离；应用重启后未完成请求标记为可重试，不伪装运行中 |
| 音乐缓存文件 | `QStandardPaths::AppDataLocation/music_cache/{hash}.{ext}` 或用户配置路径 |
| 封面缓存及解析出的嵌入封面 | `QStandardPaths::AppDataLocation` 下的文件 |

---

## 7. 健壮性要点

- **断线重连**：WsClient 指数退避重连，成功后自动重新进房 + 拉全量状态 + 重新校准时钟；
- **时钟漂移**：ClockSync 每 60s 重校，SyncController 只在偏差 >500ms 时纠正；
- **下载失败**：`.tmp` 保留断点，重试从 Range 续传；校验 hash 不符则删除重来；
- **token 过期**：ApiClient 收到 401 → 清 token、关闭 WS、停止需要鉴权的请求 → 显示登录窗；
- **被踢下线**：收到 KICKED → 停播放、断 WS、清状态、弹提示回登录页；
- **幂等**：加歌单、收藏搭子等操作重复点击不产生脏数据（依赖服务端唯一索引）。

---

## 8. 实施顺序（客户端视角）

1. Qt 6.8 / CMake / `qt_add_qml_module` 骨架，主题值、`LoginWindow.qml`、`AppWindow.qml`、导航栏和固定播放条；
2. ApiClient、AuthService、账号密码登录、自动登录与注册弹窗；
3. PlayerEngine、曲库列表、封面和歌词读取、歌曲上传弹窗与音频下载缓存；
4. QueueDrawer、LyricPage、列表行与播放条状态绑定，完成参考图对应的三种主视觉状态；
5. WsClient + ClockSync + SyncController + 房间页，跑通双人和多人同步、授权控制与断线恢复；
6. 搭子搜索/收藏、邀请流程和 InviteToast；个人及服务器歌单、批量加歌与排序；
7. 管理页面（权限/角色/密码/踢人），再完善窗口缩放、键盘焦点、高 DPI、空状态和错误提示。

---

## 9. 歌曲行、弹层与双队列实现补充（2026-10-09）

### 9.1 可复用视觉组件

当前工程的实际窗口入口为 `qml/Main.qml`，业务协调为 `src/AppController.*`。`PopupSurface` 管理圆角、主题边框和阴影；`FutariMenu`、`FutariMenuItem`、`FutariMenuSeparator` 管理菜单间距、行高、SVG 图标、悬停/禁用状态和淡入淡出；`FutariDialog` 管理标题栏、关闭操作、主题遮罩及标准按钮。上传、歌单和危险操作弹层复用这些组件，采用 `Popup.Item`。文件与目录选择继续交由 Qt 原生选择器处理。

`SongRow` 的四按钮为喜欢、下载、添加和更多，只有悬停、键盘焦点或菜单打开时显示。按钮区预留宽度，避免动画引起曲目信息横向跳动。喜欢复用「我喜欢」个人歌单；加号菜单列出现有歌单，并允许创建歌单后添加当前歌曲。更多菜单包含播放、下一首播放、下载及管理员编辑。所有操作通过 C++ 控制器复用鉴权与 REST/WebSocket 接口。

主窗口使用透明 alpha buffer，清空样式默认背景，内容层由抗锯齿圆角矩形作为 `MultiEffect` 蒙版裁剪，描边单独绘制。蒙版下阈值为 0.5、spread 为 1，保留透明角落及边缘渐变；最大化时关闭圆角层。此实现替换了整数 `QRegion` 裁剪，减少高 DPI 下的阶梯状边缘。SVG 统一使用白色素材，由主题颜色着色。

### 9.2 场景切换与权限

- 本地队列保存在 `AppController.localQueue`；房间共享队列来自 `roomState.songIds`，两者分别维护。
- 应用仅持有一份 `AudioPlayer`。创建/加入房间及接受邀请时保存个人曲目、位置，播放器跟随共享房间。离开房间恢复个人歌曲与位置，保持暂停。
- 有房间控制权时，歌曲行默认播放、加队列和下一首播放都操作当前房间；菜单另提供显式添加本地队列。无控制权时禁用共享播放操作，默认添加本地队列。
- 房间页与队列面板都读取 `roomState.songIds`；共同调用增删排序方法发布 `PLAYLIST_UPDATE`。房间内切换查看本地队列时只读，避免控件误操作当前房间。
- `PLAY/PAUSE/SEEK/NEXT/SYNC` 广播同时更新 `roomState.playback` 和播放器，防止「下一首播放」读旧歌曲。晚到的 REST 回包按播放时间戳合并；歌曲信息异步回包校验当前房间及播放状态。
- 「下一首播放」先去除该歌曲的旧队列位置，再插到当前歌曲之后；当前曲目不在队列时放到队首。当前正在播放的歌曲重复请求不改变队列。

### 9.3 验证

独立工程 `tests/playback-ui` 使用 Qt Test、真实 C++ 控制器和实际 QML，在随机本机端口的 HTTP/WebSocket 替身服务上验证：队列分离、权限、播放/下一首路由、离房暂停恢复、新建歌单并加歌、喜欢/取消喜欢。加载全窗口并在 Windows 原生图形后端验证圆角透明度及抗锯齿覆盖率、歌曲悬停动作、明暗菜单、单曲/批量上传弹层。测试会隔离设置并将音量置零，不连接正式数据库；不能替代真实双客户端的部署联调。

## 10. 上传与歌词状态隔离（2026-10-09）

`MultipartRequest.h` 统一设置带引号并转义的 multipart boundary，保留 Qt 实际消息体的边界值。`ApiClient.upload/putUpload` 与 `TransferManager.startUpload` 共用该方法，避免自动边界包含斜杠时 Spring 拒绝解析。

`TransferPage.currentRows()` 在活动 Tab 中排除 `success/cancelled`，保留失败任务的重试入口。上传完成时把 `transferred/totalBytes` 统一为音频文件大小，避免把 multipart 附加字段大小计入已完成文件的显示总量。完成记录仍保存在统一任务管理器中。

`AppController` 监听 `AudioPlayer.songChanged` 读取歌词，递增 `m_lyricsGeneration`，立即清空旧文本。响应同时校验请求代次、当前歌曲 ID 和 `SongLyricsResponse.songId`；会话清理也失效旧代次。本地歌曲无服务器 ID 时只清空歌词，不发送无效请求。`LyricsPage` 不再另行发起重复请求。

批量上传编辑器在切换选择前打开回写保护，提交时保存当前行。回归从实际 QML 匹配两个同目录 LRC，分别编辑、切换并上传，检查 multipart 消息体互不包含对方歌词；通过延迟旧曲目的歌词回包验证当前歌曲不被覆盖。还验证失败任务保留、取消移除、成功上传/下载只出现在相应历史 Tab。测试程序可接受一个本地音频文件路径，追加完整文件上传及消息体字节一致性检查。

批量解析尚未结束时禁止确认提交，避免用户确认之后异步解析才改变歌曲信息。客户端与任务管理器显式采用 `QSettings::defaultFormat()` 构造设置存储；生产默认仍使用 Windows 原生存储，测试指定临时 INI 目录。仅调用 `setDefaultFormat()` 而仍使用双字符串构造函数不能隔离 Windows 注册表。回归检查实际设置文件路径位于临时目录，避免污染本机账户、目录和任务历史。
