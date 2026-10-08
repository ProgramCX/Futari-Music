package com.chengxu.futari.common.error;

import lombok.Getter;

/** 统一业务错误码。 @author Futari */
@Getter
public enum ErrorCode {
    INTERNAL(1000, "服务器暂时不可用"), PARAM(1001, "参数错误"), UNAUTHORIZED(1002, "请先登录"), FORBIDDEN(1003, "没有操作权限"), NOT_FOUND(1004, "资源不存在"),
    USER_EXISTS(2001, "用户名已存在"), BAD_CREDENTIALS(2002, "用户名或密码错误"), USER_NOT_FOUND(2003, "用户不存在"),
    SONG_FORMAT(3001, "不支持的音乐格式"), SONG_EMPTY(3002, "文件不能为空"), SONG_STORAGE(3003, "音乐文件处理失败"), SONG_LYRICS_FORMAT(3004, "歌词仅支持 UTF-8 的 lrc 或 txt 文件"), SONG_LYRICS_SIZE(3005, "歌词不能超过 256 KB"), SONG_COVER_FORMAT(3006, "封面仅支持 JPEG、PNG 或 WebP 图片"), SONG_COVER_SIZE(3007, "封面不能超过 5 MB"), SONG_DUPLICATE(3008, "该音频文件已存在于曲库"), ALBUM_EXISTS(3009, "同名艺术家专辑已存在"), ALBUM_AMBIGUOUS(3010, "存在多个同名专辑，请先选择专辑"), SONG_UPLOAD_SIZE(3011, "上传内容超过服务器配置的大小限制，请调整文件大小或联系管理员"),
    PLAYLIST_NOT_FOUND(4001, "歌单不存在"), PLAYLIST_ORDER(4002, "排序歌曲与歌单不一致"),
    ROOM_NOT_FOUND(5001, "房间不存在"), ROOM_NOT_MEMBER(5002, "请先加入房间"), ROOM_CONTROL(5003, "没有播放控制权"), ROOM_SONG(5004, "歌曲不在房间列表中"), ROOM_ALREADY_JOINED(5005, "已在其他房间"), ROOM_OWNER_ONLY(5006, "仅房间创建者可以解散房间"),
    PARTNER_SELF(6001, "不能收藏自己"), PARTNER_OFFLINE(6002, "对方不在线，无法邀请"), INVITE_EXPIRED(6003, "邀请已失效"), INVITE_TARGET(6004, "邀请不属于当前用户"),
    ADMIN_LAST(7001, "不能撤销最后一位管理员");
    private final int code;
    private final String message;
    ErrorCode(int code, String message) { this.code = code; this.message = message; }
}
