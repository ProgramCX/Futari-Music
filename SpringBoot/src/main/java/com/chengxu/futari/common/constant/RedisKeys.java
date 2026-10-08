package com.chengxu.futari.common.constant;

/** Redis 键及生命周期常量。 @author Futari */
public final class RedisKeys {
    public static final long INVITE_TTL_SECONDS = 60;
    public static final long ROOM_IDLE_TTL_SECONDS = 600;
    public static final long ONLINE_TTL_SECONDS = 120;
    private RedisKeys() { }
    public static String login(Long userId) { return "futari:login:" + userId; }
    public static String online(Long userId) { return "futari:online:" + userId; }
    public static String roomState(Long roomId) { return "futari:room:" + roomId + ":state"; }
    public static String roomMembers(Long roomId) { return "futari:room:" + roomId + ":members"; }
    public static String roomPlaylist(Long roomId) { return "futari:room:" + roomId + ":playlist"; }
    public static String roomControllers(Long roomId) { return "futari:room:" + roomId + ":controllers"; }
    public static String userRoom(Long userId) { return "futari:user:" + userId + ":room"; }
    public static String invite(String id) { return "futari:invite:" + id; }
    public static String userInvites(Long userId) { return "futari:user:" + userId + ":invites"; }
}
