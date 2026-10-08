package com.chengxu.futari.sync;

import com.chengxu.futari.common.constant.RedisKeys;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.io.IOException;
import java.time.Duration;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;

/** WebSocket 会话与单播、房间广播。 @author Futari */
@Slf4j @Component @RequiredArgsConstructor
public class SyncPublisher {
    private final ConcurrentHashMap<Long, WebSocketSession> sessions = new ConcurrentHashMap<>();
    private final StringRedisTemplate redis;
    private final ObjectMapper json;
    public void connected(Long userId, WebSocketSession session) {
        WebSocketSession old = sessions.put(userId, session);
        if (old != null && old.isOpen()) try { old.close(); } catch (IOException ex) { log.warn("旧连接关闭失败，用户 {}", userId, ex); }
        redis.opsForValue().set(RedisKeys.online(userId), session.getId(), Duration.ofSeconds(RedisKeys.ONLINE_TTL_SECONDS));
    }
    public void disconnected(Long userId, WebSocketSession session) {
        sessions.remove(userId, session);
        String key = RedisKeys.online(userId);
        if (session.getId().equals(redis.opsForValue().get(key))) redis.delete(key);
    }
    public void heartbeat(Long userId, WebSocketSession session) {
        if (session.equals(sessions.get(userId))) redis.opsForValue().set(RedisKeys.online(userId), session.getId(), Duration.ofSeconds(RedisKeys.ONLINE_TTL_SECONDS));
    }
    public void send(Long userId, String type, Object data) {
        WebSocketSession session = sessions.get(userId);
        if (session == null) return;
        try {
            String payload = json.writeValueAsString(new WsEnvelope(type, System.currentTimeMillis(), data));
            synchronized (session) { if (session.isOpen()) session.sendMessage(new TextMessage(payload)); }
        } catch (IOException ex) {
            log.warn("消息发送失败，用户 {}", userId, ex);
            disconnected(userId, session);
            try { session.close(); } catch (IOException closeEx) { log.warn("连接关闭失败，用户 {}", userId, closeEx); }
        }
    }
    public void broadcast(Long roomId, String type, Object data) {
        Set<String> members = redis.opsForSet().members(RedisKeys.roomMembers(roomId));
        if (members != null) for (String userId : members) send(Long.valueOf(userId), type, data);
    }
    public Set<Long> onlineUserIds() { return Set.copyOf(sessions.keySet()); }
    public void close(Long userId) {
        WebSocketSession session = sessions.get(userId);
        if (session != null) try { session.close(); } catch (IOException ex) { log.warn("连接关闭失败，用户 {}", userId, ex); }
    }
}
