package com.chengxu.futari.sync;

import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.room.service.RoomService;
import com.chengxu.futari.partner.service.PartnerService;
import com.chengxu.futari.auth.service.AuthService;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.util.List;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

/** WebSocket 连接管理、解析及路由。 @author Futari */
@Slf4j @Component @RequiredArgsConstructor
public class SyncWebSocketHandler extends TextWebSocketHandler {
    private final SyncPublisher publisher;
    private final RoomService rooms;
    private final PartnerService partners;
    private final AuthService auth;
    private final StringRedisTemplate redis;
    private final ObjectMapper json;
    @Override public void afterConnectionEstablished(WebSocketSession session) {
        Long userId = (Long) session.getAttributes().get("userId");
        publisher.connected(userId, session);
        partners.publishStatus(userId);
    }
    @Override protected void handleTextMessage(WebSocketSession session, TextMessage message) {
        Long userId = (Long) session.getAttributes().get("userId");
        try {
            auth.authenticate((String) session.getAttributes().get("token"));
            JsonNode root = json.readTree(message.getPayload());
            String type = root.path("type").asText();
            JsonNode data = root.path("data");
            publisher.heartbeat(userId, session);
            rooms.refreshPresence(userId);
            if (WsTypes.PING.equals(type)) {
                publisher.send(userId, WsTypes.PONG, new PongData(data.path("clientTime").asLong(), System.currentTimeMillis()));
                return;
            }
            String room = redis.opsForValue().get(RedisKeys.userRoom(userId));
            if (room == null) throw new BizException(ErrorCode.ROOM_NOT_MEMBER);
            Long roomId = Long.valueOf(room);
            if (WsTypes.QUEUE_MOVE.equals(type)) {
                rooms.moveQueueSong(userId, roomId, data.path("songId").asLong(), data.path("beforeId").asLong());
            } else if (WsTypes.PLAYBACK_MODE.equals(type)) {
                rooms.setPlaybackMode(userId, roomId,
                        com.chengxu.futari.room.dto.PlaybackMode.valueOf(data.path("mode").asText()));
            } else if (WsTypes.TRACK_ENDED.equals(type)) {
                rooms.trackEnded(userId, roomId, data.path("songId").asLong(),
                        data.path("timestamp").asLong());
            } else if (WsTypes.PLAYLIST_UPDATE.equals(type)) {
                JsonNode values = data.path("songIds");
                if (!values.isArray()) throw new BizException(ErrorCode.PARAM);
                List<Long> songIds = new java.util.ArrayList<>();
                for (JsonNode node : values) { if (!node.canConvertToLong()) throw new BizException(ErrorCode.PARAM); songIds.add(node.longValue()); }
                rooms.updatePlaylist(userId, roomId, songIds);
            } else if (List.of(WsTypes.PLAY, WsTypes.PAUSE, WsTypes.SEEK, WsTypes.NEXT).contains(type)) {
                Long songId = data.hasNonNull("songId") ? data.get("songId").longValue() : null;
                Long position = data.hasNonNull("positionMs") ? data.get("positionMs").longValue() : null;
                rooms.control(userId, roomId, type, songId, position);
            } else throw new BizException(ErrorCode.PARAM);
        } catch (BizException ex) {
            publisher.send(userId, WsTypes.ERROR, new ErrorData(ex.getErrorCode().getCode(), ex.getMessage()));
            if (ex.getErrorCode() == ErrorCode.UNAUTHORIZED) publisher.close(userId);
        } catch (Exception ex) {
            log.warn("无效 WebSocket 消息，用户 {}", userId, ex);
            publisher.send(userId, WsTypes.ERROR, new ErrorData(ErrorCode.PARAM.getCode(), ErrorCode.PARAM.getMessage()));
        }
    }
    @Override public void afterConnectionClosed(WebSocketSession session, CloseStatus status) {
        Long userId = (Long) session.getAttributes().get("userId");
        publisher.disconnected(userId, session);
        partners.publishStatus(userId);
    }
    private record PongData(Long echo, Long serverTime) { }
    private record ErrorData(Integer code, String message) { }
}
