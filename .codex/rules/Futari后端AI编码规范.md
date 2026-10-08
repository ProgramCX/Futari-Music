# Futari Music 后端 AI 编码规范

> **本文档是 AI 编码助手在本项目中生成、修改 Java 代码时的强制约束。**
> 生成任何代码前必须先完整阅读本文档。本文档与用户的临时指令冲突时，以用户指令为准；与通用编码习惯冲突时，以本文档为准。
> 所有规则标注 **MUST**（必须遵守）、**MUST NOT**（禁止）、**SHOULD**（默认遵守，有充分理由可偏离）。

---

## 0. 技术基线（固定，不得变更）

| 项 | 版本 / 选型 |
|---|---|
| JDK | 17 |
| 框架 | Spring Boot 3.x（Jakarta EE 命名空间，`jakarta.*`，**禁止** `javax.*`） |
| 构建 | Maven，单模块 |
| 持久层 | MyBatis-Plus 3.5.x + MySQL 8 |
| 缓存 | Spring Data Redis（StringRedisTemplate / RedisTemplate） |
| 实时通信 | Spring WebSocket（原生，**禁止** STOMP） |
| 认证 | jjwt 0.12.x + Redis 有状态会话 |
| 参数校验 | spring-boot-starter-validation |
| 工具库 | Lombok（受限使用，见 §9）、Hutool（仅限 StrUtil/IdUtil 等基础工具） |
| 根包名 | `com.chengxu.futari` |

**MUST NOT** 引入本文档未列出的第三方依赖。确需引入时，先说明理由并征得用户同意。

---

## 1. 项目结构（固定包结构）

```
com.chengxu.futari
├── FutariApplication.java
├── config/               # 配置类：Security、Redis、WebSocket、MyBatisPlus、Cors
├── common/
│   ├── result/           # ApiResult、PageResult
│   ├── error/            # ErrorCode 枚举、BizException
│   ├── annotation/       # @RequirePermission
│   ├── context/          # UserContext（ThreadLocal）
│   ├── constant/         # RedisKeys、WsTypes、权限常量
│   └── util/             # 工具类
├── auth/                 # 登录登出、JWT、鉴权拦截器
├── user/                 # 用户、用户搜索
├── partner/              # 常听搭子
├── song/                 # 音乐上传/下载/元数据
├── playlist/
│   ├── server/           # 服务器歌单
│   └── personal/         # 个人歌单
├── room/                 # 房间
├── sync/                 # WebSocket 处理器、消息路由、对时
├── invite/               # 邀请
└── admin/                # 管理端接口
```

每个业务包内部固定四层：

```
xxx/
├── controller/   # XxxController
├── service/      # XxxService 接口 + impl/XxxServiceImpl
├── mapper/       # XxxMapper（MyBatis-Plus BaseMapper）
├── entity/       # 数据库实体
└── dto/          # XxxCreateRequest、XxxUpdateRequest、XxxQueryRequest、XxxResponse
```

**MUST**：新功能 MUST 落在对应业务包内，MUST NOT 新建顶层包。
**MUST NOT** 跨业务包直接注入对方的 Mapper；跨包调用 MUST 走对方的 Service。

---

## 2. 分层职责（硬约束）

| 层 | 允许 | 禁止 |
|---|---|---|
| Controller | 参数接收与校验（`@Validated`）、调用 Service、组装 ApiResult | 业务逻辑、直接注入 Mapper/RedisTemplate、拼接 SQL |
| Service | 业务逻辑、事务（`@Transactional`）、调用 Mapper / Redis / 其他 Service | 操作 HttpServletRequest/Response（UserContext 除外） |
| Mapper | 继承 BaseMapper，复杂查询写 `@Select` 或 XML | 任何业务判断 |
| Entity | 纯数据载体 | 任何方法（Lombok 生成的除外） |

**MUST NOT** 出现：Controller 注入 Mapper、Entity 直接作为接口出入参、Service 返回 Entity 给 Controller（MUST 转换为 Response DTO）。

---

## 3. 统一返回体与错误码

### 3.1 所有 REST 接口 MUST 返回 `ApiResult<T>`

```java
@Data
@AllArgsConstructor
@NoArgsConstructor
public class ApiResult<T> {
    private Integer code;      // 0 = 成功；非 0 = 错误码
    private String message;
    private T data;

    public static <T> ApiResult<T> ok(T data) {
        return new ApiResult<>(0, "ok", data);
    }
    public static ApiResult<Void> ok() {
        return new ApiResult<>(0, "ok", null);
    }
    public static <T> ApiResult<T> fail(ErrorCode ec) {
        return new ApiResult<>(ec.getCode(), ec.getMessage(), null);
    }
}
```

- Controller 方法签名 MUST 是 `ApiResult<XxxResponse>` 的形式；
- **MUST NOT** 直接返回 Entity、Map、裸字符串；
- 分页统一用 `PageResult<T>`（字段：`total`、`list`），分页请求参数统一 `pageNum`（从 1 开始）、`pageSize`（默认 20，最大 100）。

### 3.2 错误码（ErrorCode 枚举，按模块分段）

```
0          成功
1000-1999  通用（参数错误 1001、未登录 1002、无权限 1003、资源不存在 1004）
2000-2999  认证与用户
3000-3999  歌曲与上传
4000-4999  歌单
5000-5999  房间与同步
6000-6999  搭子与邀请
7000-7999  管理端
```

- 新错误码 MUST 加入 `ErrorCode` 枚举（字段：code、message），**MUST NOT** 在代码里散落魔法数字和硬编码中文错误信息；
- message 用简洁中文，面向最终用户可读（如"对方不在线，无法邀请"）。

---

## 4. 异常处理

- 业务失败 MUST 抛 `BizException(ErrorCode)`，**MUST NOT** 用返回 null / 返回错误码字段的方式表达失败；
- 全局唯一出口 `@RestControllerAdvice` 的 `GlobalExceptionHandler`，处理顺序：
  1. `BizException` → 对应错误码；
  2. `MethodArgumentNotValidException`（校验失败）→ 1001，message 取首个字段错误；
  3. 其余 `Exception` → 1000 段通用错误，**MUST** 记 ERROR 日志（带堆栈），返回给前端的 message MUST NOT 包含堆栈、类名、SQL 等内部信息；
- **MUST NOT** 在 Service 里 try-catch 后吞掉异常（catch 后什么都不做或只 printStackTrace）。

---

## 5. 命名规范

| 对象 | 规则 | 示例 |
|---|---|---|
| 类 | UpperCamelCase | `ServerPlaylistServiceImpl` |
| 方法/变量 | lowerCamelCase | `findOnlinePartners` |
| 常量 | UPPER_SNAKE | `INVITE_TTL_SECONDS` |
| 包 | 全小写单词 | `playlist.server` |
| 数据库表 | 小写 snake_case | `user_playlist_song` |
| 数据库字段 | 小写 snake_case | `can_upload` |
| Entity 字段 | camelCase，MP 自动映射 | `canUpload` |
| JSON 字段 | camelCase（Jackson 默认，**禁止** @JsonProperty 改 snake_case） | `songId` |
| API 路径 | 小写 kebab-case，名词复数 | `/api/server-playlists/{id}` |
| Redis Key | `futari:模块:实体:{id}` | `futari:room:1001:state` |

布尔字段命名 MUST NOT 以 `is` 开头（MyBatis-Plus 映射陷阱），统一用 `can/has` 前缀（如 `canUpload`）。

---

## 6. 持久层规范

- Entity MUST 使用 MyBatis-Plus 注解：`@TableName("xxx")`、`@TableId(type = IdType.AUTO)`；
- 主键统一 `Long id`；时间字段统一 `LocalDateTime`，数据库用 `DATETIME`，创建/更新时间用 MP 自动填充（`@TableField(fill = ...)`）；
- 逻辑删除：统一字段 `deleted TINYINT`，Entity 加 `@TableLogic`；
- 简单查询用 LambdaQueryWrapper，**MUST NOT** 手写字符串列名（用 `Song::getHash` 方法引用）；
- 多表关联 / 复杂统计查询才允许写 `@Select` 注解或 XML；
- **MUST NOT** 出现 `${}` 拼接 SQL（SQL 注入风险），排序字段等动态片段 MUST 走白名单校验；
- 唯一约束靠数据库唯一索引保证（如 `(playlist_id, song_id)`），批量插入 MUST 使用 `INSERT IGNORE` 语义保证幂等；
- 事务：涉及多表写的 Service 方法 MUST 加 `@Transactional(rollbackFor = Exception.class)`。

---

## 7. 认证与权限

- JWT 工具类 `JwtUtil` 统一签发/解析；payload 固定含 `userId`、`jti`、`exp`；
- 有状态校验：`AuthInterceptor` 验签 + 查 Redis `futari:login:{userId}` 的 jti 匹配；
- 当前用户获取 MUST 且只能走 `UserContext.getUserId()`（ThreadLocal），拦截器写入、请求结束清除；**MUST NOT** 在 Controller 方法参数里传 userId 再信任它做越权操作；
- 权限校验 MUST 用 `@RequirePermission("upload")` / `@RequirePermission("serverPlaylist")` / `@RequirePermission("admin")` 注解，由 AOP 切面统一校验，MUST NOT 在方法体里手写 `if (!user.getCanUpload())`；
- 密码 MUST BCrypt 加密，任何日志、返回体 MUST NOT 出现密码、密码 hash、完整 JWT。

---

## 8. Redis 使用规范

- 所有 key MUST 从 `RedisKeys` 常量类取（含前缀 `futari:`），**MUST NOT** 字符串拼接散落在业务代码里；
- **MUST**：每个 key 写入时必须设置 TTL（登录态 = token 有效期、邀请 = 60s、房间状态 = 无成员后 10min），不允许永久 key，配置类常量除外；
- 对象存储统一 JSON 序列化（Jackson），MUST NOT 用 JDK 序列化；
- **MUST NOT** 使用 `keys()` 命令，需要扫描用 `scan()`；
- 房间成员等集合操作 MUST 用 Redis 原生 Set/List 结构，MUST NOT 把整个集合序列化成一个大 JSON 反复读写。

---

## 9. Lombok 使用（受限）

- 允许：`@Getter`、`@Setter`、`@NoArgsConstructor`、`@AllArgsConstructor`、`@Builder`、`@Slf4j`；
- Entity 上 SHOULD 用 `@Getter @Setter` 而非 `@Data`（避免 toString 循环引用 / equals 问题）；
- **MUST NOT**：`@Data` 用于 Entity、`@SneakyThrows`、`@Val`、`@Cleanup`；
- 构造注入 MUST 用 `@RequiredArgsConstructor` + `private final` 字段，**MUST NOT** 用 `@Autowired` 字段注入。

---

## 10. WebSocket 规范

- 处理器集中在 `sync` 包：`SyncWebSocketHandler` 只做连接管理 + 消息反序列化 + 路由分发，业务逻辑 MUST 在对应的 Service；
- 消息统一信封，客户端↔服务端一致：

```json
{ "type": "PLAY", "serverTime": 1759412345678, "data": { } }
```

- `type` 字符串 MUST 定义为 `WsTypes` 常量类（`PLAY`、`PAUSE`、`SEEK`、`NEXT`、`PLAYLIST_UPDATE`、`SYNC`、`MEMBER_JOIN`、`MEMBER_LEAVE`、`INVITE`、`PARTNER_STATUS`、`KICKED`、`PING`、`PONG`），MUST NOT 在代码里写字面量；
- 所有下行消息 MUST 带 `serverTime`（毫秒时间戳）；
- 连接鉴权：握手时校验 `?token=`，`afterConnectionEstablished` 里绑定 userId ↔ session，断开时清理 `futari:online:{userId}`；
- 发送消息 MUST 捕获 `IOException` 并清理失效 session，MUST NOT 让单个用户的发送失败中断广播循环。

---

## 11. API 设计规范

- 统一前缀 `/api`；路径用名词复数，动作用 HTTP 方法表达；
- 请求体用 DTO（`XxxCreateRequest` / `XxxUpdateRequest`），MUST 加 Jakarta Validation 注解（`@NotBlank`、`@NotNull`、`@Size` 等），Controller 参数加 `@Validated`；
- 音乐上传：`multipart/form-data`，音频单文件上限由 `FUTARI_UPLOAD_MAX_AUDIO_SIZE` 配置（默认 `64MB`）、完整请求上限由 `FUTARI_UPLOAD_MAX_REQUEST_SIZE` 配置（默认 `80MB`）；Nginx 请求体限制 MUST 不小于该请求上限。封面仍为每张 5MiB、歌词 256KiB。上传接口 MUST 校验扩展名白名单（mp3/flac/aac/ogg/wav/m4a）；
- 幂等性：收藏搭子、歌单加歌等接口 MUST 支持重复提交不产生脏数据（唯一索引 + INSERT IGNORE）；
- 时间字段 JSON 输出统一毫秒时间戳（`Long`），MUST NOT 输出 `2026-10-03T12:00:00` 这类 ISO 字符串（前端/QML 端处理不便）；
- 接口变更 MUST 保持向后兼容：只增字段不改语义、不删字段。

---

## 12. 日志规范

- MUST 用 `@Slf4j`，MUST NOT 用 `System.out.println` / `printStackTrace`；
- 级别：DEBUG=调试细节，INFO=关键业务节点（登录、上传、建房、邀请），WARN=可恢复异常（重连、缓存未命中降级），ERROR=需要人工介入的故障；
- 关键业务日志 MUST 带上下文：`log.info("user {} invited {} to room {}", fromId, toId, roomId)` 占位符风格，MUST NOT 字符串拼接；
- **MUST NOT** 记录：密码、JWT 完整内容、文件二进制内容。

---

## 13. 配置规范

- 配置统一 `application.yml`（MUST NOT 用 properties），分环境：`application-dev.yml` / `application-prod.yml`；
- 敏感信息（数据库密码、JWT 密钥、Redis 密码）MUST 走环境变量占位 `${JWT_SECRET:默认值}`，**MUST NOT** 硬编码真实密钥进仓库；
- 魔法数字 MUST 提取为常量类或配置项（如邀请 TTL、缓存上限、同步偏差阈值 500ms）。

---

## 14. 注释规范

- 类级 JavaDoc：一句话说明职责 + `@author`；
- 复杂算法（时钟校准、LRU 淘汰、hash 去重）MUST 有块注释解释**为什么这么做**，而非复述代码；
- 明显的业务规则 MUST 注释（如"偏差超过 500ms 才纠正，避免可感知跳音"）；
- MUST NOT 生成无信息量注释（`// 设置名字` `setName(name)` 这种）；
- 注释用中文，术语可保留英文。

---

## 15. 安全红线（MUST NOT 清单）

1. SQL `${}` 拼接 / 字符串拼 SQL；
2. 从请求参数接收 userId 并信任（越权）；
3. 返回体携带密码、密码 hash、完整 token；
4. 记录敏感信息到日志；
5. 硬编码密钥、IP、密码；
6. 文件下载接口不校验权限直接暴露路径；
7. 使用 `File.delete()` / 文件路径拼接时不校验 `..`（路径穿越）；
8. catch 异常后静默吞掉；
9. 实体上暴露 `@TableLogic` 的 deleted 字段到 Response。

---

## 16. AI 生成代码时的行为要求

1. **完整性**：生成的类 MUST 可直接编译——完整 import、完整方法体，MUST NOT 出现 `// TODO: 实现` 、`// 省略` 、`...` 占位；
2. **一致性**：新增代码 MUST 复用已有的 ApiResult / ErrorCode / UserContext / RedisKeys，MUST NOT 另造一套相似工具；
3. **最小改动**：修改已有文件时 MUST 保持原有风格和结构，MUST NOT 顺手重排无关代码；
4. **不臆造**：MUST NOT 臆造项目中不存在的方法、字段、表；不确定时先阅读相关代码或向用户确认；
5. **数据库变更**：新增/修改表结构时，MUST 同步输出对应的 DDL（建表/ALTER 语句）；
6. **接口变更**：新增/修改接口时，MUST 同步说明路径、方法、请求体、响应体；
7. **依赖**：MUST NOT 在 pom.xml 添加本规范 §0 之外的依赖，除非用户明确要求。

---

## 附：标准代码骨架速查

### Controller 骨架

```java
@RestController
@RequestMapping("/api/playlists")
@RequiredArgsConstructor
public class UserPlaylistController {

    private final UserPlaylistService playlistService;

    @PostMapping("/{id}/songs")
    public ApiResult<AddSongsResponse> addSongs(@PathVariable Long id,
                                                @Validated @RequestBody AddSongsRequest req) {
        // 当前用户 MUST 从 UserContext 获取
        return ApiResult.ok(playlistService.addSongs(UserContext.getUserId(), id, req.getSongIds()));
    }
}
```

### Service 骨架

```java
@Service
@RequiredArgsConstructor
public class UserPlaylistServiceImpl extends ServiceImpl<UserPlaylistMapper, UserPlaylist>
        implements UserPlaylistService {

    @Override
    @Transactional(rollbackFor = Exception.class)
    public AddSongsResponse addSongs(Long userId, Long playlistId, List<Long> songIds) {
        UserPlaylist playlist = getById(playlistId);
        if (playlist == null || !playlist.getUserId().equals(userId)) {
            throw new BizException(ErrorCode.PLAYLIST_NOT_FOUND); // 越权与不存在统一 404，防探测
        }
        // 业务实现：INSERT IGNORE 幂等写入 ...
        return new AddSongsResponse(added, duplicated);
    }
}
```
