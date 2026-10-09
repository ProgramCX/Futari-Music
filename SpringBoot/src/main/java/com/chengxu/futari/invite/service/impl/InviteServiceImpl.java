package com.chengxu.futari.invite.service.impl;

import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.invite.dto.InviteResponse;
import com.chengxu.futari.invite.service.InviteService;
import com.chengxu.futari.room.dto.RoomStateResponse;
import com.chengxu.futari.room.service.RoomService;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.sync.SyncPublisher;
import com.chengxu.futari.user.service.UserService;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.time.Duration;
import java.util.UUID;
import java.util.List;
import java.util.ArrayList;
import java.util.Set;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

/**
 * Redis 邀请单，过期自动失效。 @author Futari
 */
@Slf4j
@Service
@RequiredArgsConstructor
public class InviteServiceImpl implements InviteService {
    private final StringRedisTemplate redis;
    private final RoomService rooms;
    private final UserService users;
    private final SongService songs;
    private final SyncPublisher publisher;
    private final ObjectMapper json;

    public InviteResponse create(Long fromId, Long roomId, Long targetId) {
        rooms.requireMember(fromId, roomId);
        users.require(targetId);
        if (fromId.equals(targetId)) throw new BizException(ErrorCode.PARAM);
        if (!Boolean.TRUE.equals(redis.hasKey(RedisKeys.online(targetId))))
            throw new BizException(ErrorCode.PARTNER_OFFLINE);
        RoomStateResponse state = rooms.state(fromId, roomId);
        Long songId = state.getPlayback().getCurrentSongId();
        String songName = songId == null ? null : songs.require(songId).getTitle();
        String id = UUID.randomUUID().toString();
        long expires = System.currentTimeMillis() + RedisKeys.INVITE_TTL_SECONDS * 1000;
        InviteResponse response = new InviteResponse(id, roomId, state.getRoom().getName(), users.response(users.require(fromId)), songName, state.getRoom().getMemberCount(), expires);
        try {
            redis.opsForValue().set(RedisKeys.invite(id), json.writeValueAsString(new StoredInvite(targetId, response)), Duration.ofSeconds(RedisKeys.INVITE_TTL_SECONDS));
        } catch (JsonProcessingException ex) {
            throw new IllegalStateException("邀请序列化失败", ex);
        }
        redis.opsForSet().add(RedisKeys.userInvites(targetId), id);
        redis.expire(RedisKeys.userInvites(targetId), Duration.ofSeconds(RedisKeys.INVITE_TTL_SECONDS));
        publisher.send(targetId, WsTypes.INVITE, response);
        log.info("user {} invited {} to room {}", fromId, targetId, roomId);
        return response;
    }

    public RoomStateResponse accept(Long userId, String inviteId) {
        StoredInvite invite = load(userId, inviteId);
        if (redis.opsForValue().getAndDelete(RedisKeys.invite(inviteId)) == null)
            throw new BizException(ErrorCode.INVITE_EXPIRED);
        redis.opsForSet().remove(RedisKeys.userInvites(userId), inviteId);
        return rooms.join(userId, invite.response().getRoomId());
    }

    public void decline(Long userId, String inviteId) {
        load(userId, inviteId);
        redis.delete(RedisKeys.invite(inviteId));
        redis.opsForSet().remove(RedisKeys.userInvites(userId), inviteId);
    }

    public List<InviteResponse> pending(Long userId) {
        Set<String> ids = redis.opsForSet().members(RedisKeys.userInvites(userId));
        if (ids == null || ids.isEmpty()) return List.of();
        List<InviteResponse> result = new ArrayList<>();
        for (String id : ids) {
            String value = redis.opsForValue().get(RedisKeys.invite(id));
            if (value == null) {
                redis.opsForSet().remove(RedisKeys.userInvites(userId), id);
                continue;
            }
            try {
                StoredInvite invite = json.readValue(value, StoredInvite.class);
                if (userId.equals(invite.targetId())) result.add(invite.response());
            } catch (JsonProcessingException ex) {
                throw new IllegalStateException("无效邀请记录", ex);
            }
        }
        result.sort((left, right) -> Long.compare(left.getExpiresAt(), right.getExpiresAt()));
        return result;
    }

    private StoredInvite load(Long userId, String inviteId) {
        if (inviteId == null || !inviteId.matches("[a-f0-9-]{36}")) throw new BizException(ErrorCode.INVITE_EXPIRED);
        String value = redis.opsForValue().get(RedisKeys.invite(inviteId));
        if (value == null) throw new BizException(ErrorCode.INVITE_EXPIRED);
        try {
            StoredInvite invite = json.readValue(value, StoredInvite.class);
            if (!userId.equals(invite.targetId())) throw new BizException(ErrorCode.INVITE_TARGET);
            return invite;
        } catch (JsonProcessingException ex) {
            throw new IllegalStateException("无效邀请记录", ex);
        }
    }

    private record StoredInvite(Long targetId, InviteResponse response) {
    }
}
