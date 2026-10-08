package com.chengxu.futari.common.constant;

/** WebSocket 消息类型。 @author Futari */
public final class WsTypes {
    private WsTypes() { }
    public static final String ROOM_DELETED = "ROOM_DELETED";
    public static final String PLAY = "PLAY", PAUSE = "PAUSE", SEEK = "SEEK", NEXT = "NEXT", PLAYLIST_UPDATE = "PLAYLIST_UPDATE", SYNC = "SYNC", MEMBER_JOIN = "MEMBER_JOIN", MEMBER_LEAVE = "MEMBER_LEAVE", INVITE = "INVITE", PARTNER_STATUS = "PARTNER_STATUS", KICKED = "KICKED", PING = "PING", PONG = "PONG", ERROR = "ERROR", CONTROL_UPDATE = "CONTROL_UPDATE";
}
