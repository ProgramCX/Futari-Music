# Futari Music 客户端与开源协作规范

适用于 `Qt/FutariMusic` 中的 C++17、Qt 6 和 QML 改动，以及全仓库开源协作。后端 Java 改动继续遵守同目录的《Futari后端AI编码规范》，在不冲突处参考 [Google Java Style Guide](https://google.github.io/styleguide/javaguide.html)。本规范参考 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)、[Google Engineering Practices](https://google.github.io/eng-practices/) 和 [Google 文档规范](https://google.github.io/styleguide/docguide/)；项目现有 Qt 命名和 API 约定优先于机械重命名。


## 工程与代码

- 改动前阅读相关模块和调用链，保持既有功能、接口和用户数据兼容。每个提交聚焦一个可解释的问题；不要顺手重排无关文件。
- C++ 类、函数、成员和常量采用一致的项目命名；新代码优先遵循 Google C++ 风格。已有 `m_` 成员、Qt 信号槽和 QML 暴露属性沿用当前风格，避免无意义的大规模重命名。
- 新增 C++ 代码按 `Qt/FutariMusic/.clang-format` 排版；只格式化本次修改的区域，不对历史文件进行无关的全量格式化。
- 头文件只包含所需依赖；能前向声明时使用前向声明。避免循环依赖、隐式所有权和全局可变状态。QObject 的父子关系、异步回调的生命周期和资源释放必须明确。
- 使用 RAII 管理文件、网络和其他资源；检查 I/O、网络、解析及转换结果。错误应反馈到界面，日志不包含口令、令牌或本地敏感路径。
- 函数保持单一职责，消除重复逻辑与魔法值。复杂状态转换说明原因和不变量；注释解释为什么，避免复述代码。
- 不引入与现有 Qt 模块重复的依赖。公开接口变更需同步更新调用方、文档和必要的兼容处理。

## QML 与界面

- 颜色、圆角、间距、图标状态从 `Theme.qml` 和共享组件取得。浅色与深色必须同时检查；图标使用 SVG，不能以 ASCII 字符代替。
- 鼠标可点击区域使用指针手势，并提供可见的悬停、按下、禁用与焦点状态。弹层支持点击外部和 Esc 关闭；键盘可达，不遮挡关键内容。
- 组件只维护自身展示状态；播放器、房间和传输等业务状态以对应控制器为唯一来源。异步结果需防止过期回调覆盖较新的用户操作。
- UI 文案描述真实行为。暂停、进度、完成和失败状态必须来自实际业务结果，不能仅在界面模拟。

## 变更交付

- 修改后检查差异与空白错误，构建受影响模块。对存在实际回归风险的行为进行有针对性的验证，并如实说明未验证的运行条件。
- 用户可见功能更新相应文档。提交说明写清问题、原因、改动和验证结果；代码审查中优先检查正确性、可维护性、可读性及安全性。
- README、使用说明和 API 文档使用清晰的标题、可执行的步骤及准确的链接；不把计划中的功能写成已实现。

# Qt C++ 代码规范文档

## 文档说明

本文档基于 Google C++ Style Guide 中文版和 Qt Coding Conventions，结合项目实际代码问题编写。每条规范尽可能标注依据来源和项目中的具体反例，便于对照执行。

---

## 1. 命名与格式

### 1.1 通用命名规则

命名要有描述性，避免只有项目内部人员才能理解的缩写。

```cpp
// 反例
int n;           // 毫无意义
int nerr;        // 含糊不清的缩写
QJsonObject obj; // 泛化过度

// 正例
int errorCount;
int dnsConnectionCount;
QJsonObject userProfile;
```

**项目反例**：`object()` / `array()` / `number()` 三个辅助函数以泛化名称封装了类型转换，缺乏描述性，且增加了不必要的抽象层。

### 1.2 类型命名

类型名称每个单词首字母大写，不含下划线。

```cpp
// 正例
class AudioPlayer;
struct SessionState;
enum class PlaybackStatus;
using SongIdList = QList<qint64>;
```

### 1.3 变量命名

变量名全部小写，单词之间用下划线连接。类成员变量以 `m_` 为前缀。

```cpp
// 正例
QString song_title;
bool is_loading;
// 成员变量
qint64 m_roomId;
QJsonArray m_localQueue;
```

### 1.4 函数命名

函数名使用驼峰命名（camelCase），动词开头。

```cpp
// 正例
void loadMoreSongs();
void refreshRoomState();
bool canControl() const;

// 反例
void songs();
bool control();
```

### 1.5 信号与槽命名

信号使用动词的过去式或过去分词，槽函数以 `on` 前缀。

```cpp
signals:
    void songChanged();
    void roomStateChanged();
    void infoMessage(const QString &text);

private slots:
    void onSongChanged();
    void onRoomStateChanged();
```

### 1.6 常量命名

常量以 `k` 为前缀，驼峰命名。

```cpp
// 正例
constexpr qint64 kMaxCacheBytes = 128LL * 1024 * 1024;
constexpr int kDefaultPageSize = 40;

// 反例
constexpr qint64 maxBytes = 128LL * 1024 * 1024;
```

**项目反例**：`pruneCoverCache` 中的 `maxBytes` 应改为 `kMaxCoverCacheBytes`。

### 1.7 命名空间命名

命名空间使用小写单词，不含下划线。

```cpp
namespace api { }
namespace audio { }
```

### 1.8 格式规范

- 缩进使用 4 个空格，不使用 Tab
- 行宽不超过 120 字符
- 左花括号不换行（类和方法定义除外）
- 头文件包含顺序：对应头文件 → Qt 头文件 → 标准库头文件 → 项目头文件

---

## 2. 头文件与类设计

### 2.1 头文件保护

公共头文件使用传统 include guard（如 `APP_CONTROLLER_H`），仅工具/测试代码可使用 `#pragma once`。

### 2.2 类声明顺序

类内成员声明顺序：`public` → `protected` → `private`，成员变量在所有方法之后。

### 2.3 构造函数职责

构造函数只做初始化，不执行业务逻辑。复杂初始化拆分为独立的 `setup` 方法。

```cpp
// 反例：一个构造函数 100+ 行，包含 connect、定时器、token 恢复等
AppController::AppController(QObject *parent)
    : QObject(parent), m_api(this), ... {
    setServerUrl(...);
    connect(&m_api, ...);
    connect(&m_player, ...);
    // ... 100+ 行
}

// 正例
AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_api(this)
    , m_player(&m_api, this)
{
    setupTimers();
    setupSocket();
    setupConnections();
    restoreSession();
}
```

### 2.4 初始化列表

所有成员变量必须使用初始化列表初始化。

```cpp
// 正例
SessionState::SessionState()
    : userId(0)
    , authenticated(false)
    , roomId(0)
{}
```

### 2.5 结构体 vs 类

仅包含数据的聚合类型使用 `struct`；有行为（方法、不变式）的使用 `class`。

### 2.6 避免匿名命名空间中的无意义包装

Google 风格指南允许匿名命名空间，但不得用其创建冗余的类型转换包装。

```cpp
// 反例：毫无意义的包装
namespace {
QJsonObject object(const QJsonValue &v) { return v.toObject(); }
QJsonArray array(const QJsonValue &v) { return v.toArray(); }
qint64 number(const QJsonValue &v) { return v.toVariant().toLongLong(); }
}

// 正例：直接使用 Qt API
const QJsonObject data = value.toObject();
const qint64 id = value.toInteger();
```

**原因**：`object()` / `array()` 只是 `.toObject()` / `.toArray()` 的别名，不增加任何语义价值，反而让阅读者需要额外记忆映射关系。`number()` 通过 `toVariant().toLongLong()` 绕了一圈，Qt 6 直接提供了 `QJsonValue::toInteger()`。

### 2.7 不可拷贝类型

QObject 派生类必须禁用拷贝构造和赋值（`Q_DISABLE_COPY`）。

```cpp
class AppController : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(AppController)
public:
    // ...
};
```

---

## 3. Qt 特有规范

### 3.1 Q_OBJECT 宏

每个 QObject 子类必须包含 `Q_OBJECT` 宏，即使没有信号和槽。

### 3.2 信号槽连接

统一使用新式连接语法。

```cpp
// 正例
connect(&m_api, &ApiClient::errorOccurred, this, &AppController::setError);
connect(&m_player, &AudioPlayer::songChanged, this, &AppController::onSongChanged);

// 反例
connect(&m_api, SIGNAL(errorOccurred(QString)), this, SLOT(setError(QString)));
```

### 3.3 信号参数

- 信号参数使用 const 引用传递复杂类型
- 信号不要使用 `const` 关键字修饰（Qt 元对象系统兼容性考虑）

### 3.4 错误处理

Qt 本身不抛异常，使用错误码和错误信号。

```cpp
// 正例：通过信号传递错误
signals:
    void errorOccurred(const QString &message);

// 反例：抛出异常
throw std::runtime_error("connection failed");
```

### 3.5 智能指针与所有权

使用 `QScopedPointer` 或 `std::unique_ptr` 管理资源，避免裸 `new`/`delete`。

```cpp
// 正例
QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply;
```

### 3.6 避免布尔参数

方法参数中避免裸 `bool`，使用枚举替代。

```cpp
// 反例
void updatePlaylist(qint64 id, const QString &name, ..., bool server);

// 正例
enum class PlaylistScope { User, Server };
void updatePlaylist(qint64 id, const QString &name, ..., PlaylistScope scope);
```

**项目反例**：`updatePlaylist`、`addSongsToPlaylist`、`deletePlaylist` 等方法中的 `bool server` 参数应改为 `PlaylistScope` 枚举。

---

## 4. 异步请求与状态管理

### 4.1 分页请求统一封装

避免每个分页接口手写 URL 拼接和 generation 检查。封装统一的分页请求方法。

```cpp
// 正例：统一的分页请求
class ApiClient {
public:
    template <typename Callback>
    void pagedGet(const QString &path, int page, int pageSize,
                  const QUrlQuery &extra = {},
                  Callback &&onSuccess);
};

// 调用
m_api.pagedGet("api/songs", page, kDefaultPageSize,
               {{"keyword", m_songQuery}},
               [this](const QJsonObject &result) {
    m_songTotal = result.value("total").toInt();
    appendItems(result.value("list").toArray(), m_songs);
    emit songsChanged();
});
```

**项目反例**：`loadMoreSongs`、`loadMoreRooms`、`loadMoreUsers`、`loadMoreAdminUsers` 四个方法结构几乎完全一致，都是手拼 URL + generation 检查 + 解析 total/list 的重复模式。

### 4.2 请求取消替代 Generation 计数器

使用请求 ID 或 `QPointer` 替代每个数据源各自维护的 `m_xxxGeneration` 计数器。Qt 6 提供了 `QFuture::cancelChain()` 来传播取消信号。

```cpp
// 正例：基于 QPointer 的取消模式
class RequestToken {
public:
    bool isValid() const { return !m_cancelled; }
    void cancel() { m_cancelled = true; }
private:
    bool m_cancelled = false;
};

// 使用
auto token = QSharedPointer<RequestToken>::create();
m_activeRequests.insert("songs", token);
m_api.get("api/songs", [this, token](const QJsonValue &value) {
    if (!token->isValid()) return;
    // 处理响应
});
// 新请求时：m_activeRequests["songs"]->cancel();
```

**项目反例**：当前代码有 10 个独立的 `m_xxxGeneration` 成员变量（`m_songSearchGeneration`、`m_roomSearchGeneration`、`m_uploadedSongsGeneration`、`m_playlistsGeneration` 等），每个回调都手写 `if (generation != m_xxxGeneration) return;`。应统一为请求 token 机制。

### 4.3 会话状态打包

将散落的会话状态成员打包为结构体，`reset` 只需一行赋值。

```cpp
// 正例
struct SessionState {
    qint64 userId = 0;
    QString username;
    QString role;
    bool authenticated = false;
    bool canUpload = false;
    bool canManageServerPlaylist = false;
    qint64 roomId = 0;
    QJsonObject roomState;
    QJsonArray invites;
    QJsonArray localQueue;
    QHash<qint64, QJsonObject> songCache;
    // ...
};

class AppController : public QObject {
    // ...
private:
    SessionState m_session;
};

// reset 变为一行
void AppController::resetSession() {
    m_session = SessionState{};
    emit sessionChanged();
    // ...
}
```

**项目反例**：`resetSession` 方法中手动清空了 30+ 个成员变量（`m_roomId = 0; m_userId = 0; m_roomState = {}; m_invites = {}; ...`），漏掉任何一个都会产生 bug。将关联状态打包为一个结构体后，重置只需一行赋值。

### 4.4 回调嵌套展平

将多步异步操作封装为独立方法，避免三层以上回调嵌套。

```cpp
// 反例：三层嵌套
m_api.request("GET", "api/playlists", {}, [this, ...](const QJsonValue &value) {
    // ...
    m_api.request("POST", "api/playlists", {...}, [this, ...](const QJsonValue &created) {
        // ...
        m_api.request("POST", "api/playlists/" + id + "/songs", {...},
                      [this, ...](const QJsonValue &) { /* ... */ });
    });
});

// 正例：拆分为独立方法
void AppController::toggleFavorite(qint64 songId) {
    fetchPlaylistsThenUpdate(songId);
}

void AppController::fetchPlaylistsThenUpdate(qint64 songId) {
    m_api.request("GET", "api/playlists", {},
                  [this, songId](const QJsonValue &value) {
        updatePlaylists(value, songId);
    });
}

void AppController::updatePlaylists(const QJsonValue &value, qint64 songId) {
    // 单层逻辑
}
```

### 4.5 歌曲查找统一走缓存

保证所有进入 UI 的歌曲都存入 `m_songCache`，`findSong` 只做一次哈希查找。

```cpp
// 正例
QJsonObject AppController::findSong(qint64 id) const {
    return m_songCache.value(id);  // 一次哈希查找
}

// 在数据加载入口统一写入缓存
void AppController::appendSongs(const QJsonArray &songs) {
    for (const QJsonValue &song : songs) {
        const QJsonObject obj = song.toObject();
        m_songCache.insert(obj.value("id").toInteger(), obj);
        m_songs.append(obj);
    }
}
```

**项目反例**：当前 `findSong` 先查 `m_songCache`，未命中时线性扫描 `m_songs`，再未命中时遍历所有歌单的所有歌曲。这说明缓存的写入不完整，导致查找逻辑变得复杂且低效。

### 4.6 缓存文件管理

缓存文件名由业务 ID 直接推导，不依赖文件名格式约定。

```cpp
// 正例：按 ID 推导路径
QString cachePath(qint64 songId) {
    return cacheDirectory() + "/" + QString::number(songId) + ".cache";
}

// 清理时直接匹配数字文件名
// 反例：用正则匹配 SHA-256 哈希 + 扩展名
static const QRegularExpression cacheFilePattern(
    QStringLiteral("^[a-f0-9]{64}\\.(mp3|flac|aac|ogg|wav|m4a)(\\.part)?$"));
```

**项目反例**：`clearSongCache` 使用 64 位十六进制哈希 + 扩展名的正则来识别缓存文件。这意味着如果缓存格式变化，正则也需要同步修改。应由缓存层提供索引文件或统一命名规则。

---

## 5. 错误处理

### 5.1 错误信息传递

统一通过 `errorOccurred` 信号传递错误，不在深层回调中直接操作 UI。

### 5.2 错误信息内容

错误信息应包含可操作的上下文。

```cpp
// 正例
setError(QStringLiteral("下载目录不可写，请选择其他文件夹"));
setError(QStringLiteral("实时连接尚未建立"));

// 反例
setError("Error");  // 无上下文
```

### 5.3 错误恢复

错误发生后应清理相关状态，避免残留脏数据。

```cpp
// 正例
connect(&m_api, &ApiClient::errorOccurred, this, [this] {
    m_coverLoading.clear();  // 清理加载中的标记
});
```

---

## 6. 函数设计

### 6.1 函数长度

单个函数不超过 50 行。超过则拆分。

**项目反例**：`AppController::AppController` 约 70 行，`resetSession` 约 30 行密集赋值，`handleSocketMessage` 约 40 行 if-else 链。

### 6.2 参数数量

方法参数不超过 5 个。超出时使用结构体或配置对象传递。

```cpp
// 反例：8 个参数
void uploadSong(const QString &audioUrl, const QString &title,
                const QString &artist, qint64 albumId,
                const QString &newAlbumName, const QString &lyrics,
                const QString &lyricsUrl, const QString &coverUrl);

// 正例：使用结构体
struct SongUploadRequest {
    QString audioUrl;
    QString title;
    QString artist;
    qint64 albumId = 0;
    QString newAlbumName;
    QString lyrics;
    QString lyricsUrl;
    QString coverUrl;
};
void uploadSong(const SongUploadRequest &request);
```

### 6.3 const 正确性

不修改成员变量的方法必须标记为 `const`。

```cpp
// 正例
QString downloadDirectory() const;
QJsonObject findSong(qint64 id) const;
QVariantList favoriteSongIds() const;
```

### 6.4 返回值与错误码分离

使用 `std::optional` 或独立的错误通道，避免用特殊值表示错误。

```cpp
// 正例
std::optional<QJsonObject> findSong(qint64 id) const;
```

---

## 7. Lambda 与闭包

### 7.1 捕获列表最小化

只捕获需要的变量，优先按值捕获（异步场景避免悬空引用）。

```cpp
// 反例：按引用捕获局部变量，异步回调可能已失效
m_api.request("GET", path, {}, [&](const QJsonValue &value) {
    callback(value);  // callback 可能是悬空引用
});

// 正例
m_api.request("GET", path, {}, [this, callback, generation](const QJsonValue &value) {
    if (generation != m_generation) return;
    callback(value);
});
```

### 7.2 Lambda 复杂度

超过 15 行的 lambda 提取为独立方法。

**项目反例**：`toggleFavorite` 中的嵌套 lambda 如果展平后超过 30 行。

### 7.3 避免在 Lambda 中修改外部状态

Lambda 应保持纯粹，通过信号或回调传递结果。

---

## 8. 项目分层架构

### 8.1 职责分离

按 MVC 模式组织代码：Controller 处理用户交互，协调 Model 和 Service；网络请求封装在 API Client 中；数据模型独立于 UI。

```
├── controllers/       # AppController — 仅负责协调
├── services/          # ApiClient, TransferManager, AudioPlayer
├── models/            # SessionState, RoomState, PlaylistModel
├── views/             # UI 相关
└── utils/             # 通用工具
```

**项目反例**：当前 `AppController` 承担了太多职责：HTTP 请求、WebSocket 管理、音频播放控制、缓存清理、文件操作、用户管理、权限管理……应根据职责拆分为多个协调器（`RoomController`、`PlaylistController`、`AdminController` 等）。

### 8.2 网络层封装

`ApiClient` 应提供统一的请求接口，隐藏 URL 拼接、序列化、错误处理细节。

```cpp
class ApiClient : public QObject {
    Q_OBJECT
public:
    // 统一请求入口
    void get(const QString &path, const QUrlQuery &query,
             SuccessCallback onSuccess, ErrorCallback onError = {});
    void post(const QString &path, const QJsonObject &body,
              SuccessCallback onSuccess, ErrorCallback onError = {});
    void del(const QString &path, SuccessCallback onSuccess,
             ErrorCallback onError = {});

    // 分页请求
    void pagedGet(const QString &path, int page, int pageSize,
                  const QUrlQuery &extra,
                  PagedCallback onSuccess, ErrorCallback onError = {});
};
```

### 8.3 状态变更通知

通过信号通知状态变化，不在 Controller 内部直接操作 UI。

```cpp
// 正例
emit songsChanged();
emit roomStateChanged();
emit errorMessageChanged();
```

---

## 9. 代码审查 Checklist

以下 checklist 覆盖本文档所有规范要点，提交代码前逐项检查：

### 命名与格式
- [ ] 变量/函数/类命名符合 Google 风格指南
- [ ] 成员变量以 `m_` 前缀
- [ ] 常量以 `k` 前缀
- [ ] 信号使用过去式命名，槽使用 `on` 前缀
- [ ] 缩进 4 空格，行宽 ≤ 120

### 类设计
- [ ] 构造函数只做初始化，不执行业务逻辑
- [ ] 所有成员使用初始化列表
- [ ] QObject 子类包含 `Q_OBJECT` 宏
- [ ] 使用 `Q_DISABLE_COPY` 禁用拷贝
- [ ] 避免匿名命名空间中的无意义包装

### Qt 特有
- [ ] 信号槽使用新式连接语法
- [ ] 错误处理使用信号而非异常
- [ ] 避免裸 `new`/`delete`
- [ ] 布尔参数替换为枚举

### 异步与状态
- [ ] 分页请求使用统一封装
- [ ] 使用请求 token 替代 generation 计数器
- [ ] 会话状态打包为结构体
- [ ] 回调嵌套不超过 2 层
- [ ] `findSong` 等查找方法走哈希查找

### 函数设计
- [ ] 单函数不超过 50 行
- [ ] 参数不超过 5 个
- [ ] `const` 正确性
- [ ] Lambda 捕获最小化

### 架构
- [ ] Controller 职责单一
- [ ] 网络请求封装在 ApiClient
- [ ] 状态变更通过信号通知

---

## 10. 工具配置建议

### 10.1 clang-tidy

在 `.clang-tidy` 中启用以下检查：

```yaml
Checks: >
  bugprone-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers

CheckOptions:
  - key: readability-identifier-naming.MemberPrefix
    value: m_
  - key: readability-identifier-naming.ConstantPrefix
    value: k
```

### 10.2 cpplint

使用 Google 官方 cpplint 工具检查风格问题。

### 10.3 项目自定义规则

针对本项目，建议额外添加：

1. **禁止在 `anonymous namespace` 中定义类型转换包装**（`object`/`array`/`number`）
2. **禁止新增 `m_xxxGeneration` 成员**，统一使用 `RequestToken`
3. **`AppController` 新增方法不超过 3 个参数**，超出时使用结构体
4. **所有网络请求必须通过 `ApiClient`**，禁止在 Controller 中直接使用 `QNetworkAccessManager`

---

## 附录：对现有代码的具体修改建议

| 代码位置 | 问题 | 修改方向 |
|---------|------|---------|
| `namespace { object/array/number }` | 无意义包装 | 删除，直接调用 Qt API |
| `AppController::AppController` | 100+ 行构造函数 | 拆为 `setupTimers` / `setupSocket` / `setupConnections` / `restoreSession` |
| `m_xxxGeneration` × 10 | 计数器泛滥 | 统一 `RequestToken` 机制 |
| `resetSession` | 30+ 行手动清空 | 打包 `SessionState` 结构体，一行 `m_session = {}` |
| `loadMoreSongs` 等 4 个方法 | 结构重复 | 封装 `ApiClient::pagedGet` |
| `findSong` 线性扫描 | 缓存写入不完整 | 统一在数据入口写入 `m_songCache` |
| `clearSongCache` 正则匹配 | 依赖文件名约定 | 由缓存层提供索引或 ID 命名 |
| `toggleFavorite` 三层嵌套 | 回调地狱 | 拆为独立方法，展平为单层 |
| `bool server` 参数 | 布尔参数 | 改为 `enum class PlaylistScope` |
| `uploadSong` 8 个参数 | 参数过多 | 使用 `SongUploadRequest` 结构体 |