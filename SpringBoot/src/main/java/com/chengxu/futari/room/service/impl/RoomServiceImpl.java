package com.chengxu.futari.room.service.impl;

import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.room.dto.*;
import com.chengxu.futari.room.entity.Room;
import com.chengxu.futari.room.mapper.RoomMapper;
import com.chengxu.futari.room.service.RoomService;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.partner.service.PartnerService;
import com.chengxu.futari.sync.SyncPublisher;
import com.chengxu.futari.user.service.UserService;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.time.Duration;
import java.time.ZoneId;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.data.redis.core.script.DefaultRedisScript;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

/** Redis 房间热状态与 MySQL 元数据实现。 @author Futari */
@Slf4j @Service @RequiredArgsConstructor
public class RoomServiceImpl implements RoomService {
    private static final Duration ACTIVE_TTL = Duration.ofHours(24);
    private static final Duration IDLE_TTL = Duration.ofSeconds(RedisKeys.ROOM_IDLE_TTL_SECONDS);
    private static final DefaultRedisScript<Long> REMOVE_ROOM_MAPPING = new DefaultRedisScript<>(
            "if redis.call('get', KEYS[1]) == ARGV[1] then return redis.call('del', KEYS[1]) else return 0 end", Long.class);
    private final RoomMapper rooms;
    private final StringRedisTemplate redis;
    private final ObjectMapper json;
    private final UserService users;
    private final SongService songs;
    private final SyncPublisher publisher;
    private final PartnerService partners;

    public Room require(Long roomId) {
        Room room = rooms.selectById(roomId);
        if (room == null) throw new BizException(ErrorCode.ROOM_NOT_FOUND);
        return room;
    }
    @Transactional(rollbackFor = Exception.class)
    public RoomResponse create(Long userId, RoomCreateRequest request) {
        Room room = new Room(); room.setName(request.getName().trim()); room.setOwnerId(userId);
        rooms.insert(room);
        join(userId, room.getId());
        log.info("user {} created room {}", userId, room.getId());
        return summary(room);
    }
    public PageResult<RoomResponse> list(int pageNum, int pageSize) {
        if (pageNum < 1 || pageSize < 1 || pageSize > 100) throw new BizException(ErrorCode.PARAM);
        Page<Room> page = rooms.selectPage(new Page<>(pageNum, pageSize), null);
        return new PageResult<>(page.getTotal(), page.getRecords().stream().map(this::summary).toList());
    }
    private RoomResponse summary(Room room) {
        Long count = redis.opsForSet().size(RedisKeys.roomMembers(room.getId()));
        Long created = room.getCreatedAt() == null ? null : room.getCreatedAt().atZone(ZoneId.systemDefault()).toInstant().toEpochMilli();
        return new RoomResponse(room.getId(), room.getName(), room.getOwnerId(), count == null ? 0L : count, created);
    }
    public void requireMember(Long userId, Long roomId) {
        if (!Boolean.TRUE.equals(redis.opsForSet().isMember(RedisKeys.roomMembers(roomId), userId.toString()))) throw new BizException(ErrorCode.ROOM_NOT_MEMBER);
    }
    public void requireController(Long userId, Long roomId) {
        requireMember(userId, roomId);
        Room room = require(roomId);
        if (!room.getOwnerId().equals(userId) && !Boolean.TRUE.equals(redis.opsForSet().isMember(RedisKeys.roomControllers(roomId), userId.toString()))) throw new BizException(ErrorCode.ROOM_CONTROL);
    }
    public RoomStateResponse state(Long userId, Long roomId) {
        Room room = require(roomId); requireMember(userId, roomId);
        Set<String> rawMembers = redis.opsForSet().members(RedisKeys.roomMembers(roomId));
        Set<String> rawControllers = redis.opsForSet().members(RedisKeys.roomControllers(roomId));
        List<Long> controllerIds = new ArrayList<>(); controllerIds.add(room.getOwnerId());
        if (rawControllers != null) rawControllers.stream().map(Long::valueOf).filter(id -> !controllerIds.contains(id)).forEach(controllerIds::add);
        return new RoomStateResponse(summary(room), playback(roomId), playlist(roomId), rawMembers == null ? List.of() : rawMembers.stream().map(Long::valueOf).map(users::require).map(users::response).toList(), controllerIds, System.currentTimeMillis());
    }
    public RoomStateResponse current(Long userId) {
        String raw = redis.opsForValue().get(RedisKeys.userRoom(userId));
        if (raw == null) return null;
        Long roomId = Long.valueOf(raw);
        if (rooms.selectById(roomId) == null || !Boolean.TRUE.equals(redis.opsForSet().isMember(RedisKeys.roomMembers(roomId), userId.toString()))) {
            redis.execute(REMOVE_ROOM_MAPPING, List.of(RedisKeys.userRoom(userId)), raw);
            return null;
        }
        return state(userId, roomId);
    }
    public synchronized RoomStateResponse join(Long userId, Long roomId) {
        require(roomId);
        String key = RedisKeys.userRoom(userId);
        String current = redis.opsForValue().get(key);
        if (current != null && !current.equals(roomId.toString()) && rooms.selectById(Long.valueOf(current)) == null) {
            redis.execute(REMOVE_ROOM_MAPPING, List.of(key), current);
            current = redis.opsForValue().get(key);
        }
        if (current != null && !current.equals(roomId.toString())) throw new BizException(ErrorCode.ROOM_ALREADY_JOINED);
        Boolean claimed = redis.opsForValue().setIfAbsent(key, roomId.toString(), ACTIVE_TTL);
        if (Boolean.FALSE.equals(claimed) && !roomId.toString().equals(redis.opsForValue().get(key))) throw new BizException(ErrorCode.ROOM_ALREADY_JOINED);
        Long added = redis.opsForSet().add(RedisKeys.roomMembers(roomId), userId.toString());
        refreshRoom(roomId); redis.expire(key, ACTIVE_TTL);
        RoomStateResponse result = state(userId, roomId);
        if (Long.valueOf(1L).equals(added)) publisher.broadcast(roomId, WsTypes.MEMBER_JOIN, users.response(users.require(userId)));
        partners.publishStatus(userId);
        return result;
    }
    public synchronized void leave(Long userId, Long roomId) {
        requireMember(userId, roomId);
        redis.opsForSet().remove(RedisKeys.roomMembers(roomId), userId.toString());
        redis.opsForSet().remove(RedisKeys.roomControllers(roomId), userId.toString());
        String key = RedisKeys.userRoom(userId);
        if (roomId.toString().equals(redis.opsForValue().get(key))) redis.delete(key);
        Long count = redis.opsForSet().size(RedisKeys.roomMembers(roomId));
        if (count == null || count == 0) expireRoom(roomId, IDLE_TTL);
        publisher.broadcast(roomId, WsTypes.MEMBER_LEAVE, users.response(users.require(userId)));
        partners.publishStatus(userId);
    }
    public synchronized void delete(Long userId, Long roomId) {
        Room room = require(roomId);
        if (!room.getOwnerId().equals(userId)) throw new BizException(ErrorCode.ROOM_OWNER_ONLY);
        Set<String> members = redis.opsForSet().members(RedisKeys.roomMembers(roomId));
        if (rooms.deleteById(roomId) == 0) throw new BizException(ErrorCode.ROOM_NOT_FOUND);
        // 先通知仍在房间的客户端，再清理成员集合；账号登录与个人队列不受解散影响。
        publisher.broadcast(roomId, WsTypes.ROOM_DELETED, new RoomDeletedResponse(roomId));
        Set<String> affected = new LinkedHashSet<>(members == null ? Set.of() : members);
        affected.add(userId.toString());
        for (String member : affected) {
            Long memberId = Long.valueOf(member);
            // 只删除仍指向此房间的映射，不能误删用户刚加入的另一个房间。
            redis.execute(REMOVE_ROOM_MAPPING, List.of(RedisKeys.userRoom(memberId)), roomId.toString());
        }
        redis.delete(List.of(RedisKeys.roomState(roomId), RedisKeys.roomMembers(roomId),
                RedisKeys.roomPlaylist(roomId), RedisKeys.roomControllers(roomId)));
        for (String member : affected) partners.publishStatus(Long.valueOf(member));
        log.info("user {} deleted room {}", userId, roomId);
    }
    public void grantControl(Long ownerId, Long roomId, ControlUpdateRequest request) {
        Room room = require(roomId);
        if (!room.getOwnerId().equals(ownerId)) throw new BizException(ErrorCode.ROOM_CONTROL);
        requireMember(ownerId, roomId);
        requireMember(request.getMemberId(), roomId);
        if (request.getMemberId().equals(ownerId)) throw new BizException(ErrorCode.PARAM);
        String key = RedisKeys.roomControllers(roomId);
        if (request.getCanControl()) redis.opsForSet().add(key, request.getMemberId().toString());
        else redis.opsForSet().remove(key, request.getMemberId().toString());
        redis.expire(key, ACTIVE_TTL);
        publisher.broadcast(roomId, WsTypes.CONTROL_UPDATE, state(ownerId, roomId).getControllerIds());
    }
    public PlaybackState playback(Long roomId) {
        String value = redis.opsForValue().get(RedisKeys.roomState(roomId));
        if (value == null) return new PlaybackState();
        try { return json.readValue(value, PlaybackState.class); }
        catch (JsonProcessingException ex) { throw new IllegalStateException("无效房间状态", ex); }
    }
    public synchronized PlaybackState control(Long userId, Long roomId, String type, Long songId, Long positionMs) {
        requireController(userId, roomId);
        PlaybackState state = playback(roomId);
        long now = System.currentTimeMillis();
        if (positionMs != null && positionMs < 0) throw new BizException(ErrorCode.PARAM);
        switch (type) {
            case WsTypes.PLAY -> {
                if (songId == null || !playlist(roomId).contains(songId)) throw new BizException(ErrorCode.ROOM_SONG);
                state.setCurrentSongId(songId); state.setPositionMs(positionMs == null ? 0 : positionMs); state.setStatus("playing");
            }
            case WsTypes.PAUSE, WsTypes.SEEK -> {
                if (state.getCurrentSongId() == null || positionMs == null) throw new BizException(ErrorCode.PARAM);
                state.setPositionMs(positionMs);
                if (WsTypes.PAUSE.equals(type)) state.setStatus("paused");
            }
            case WsTypes.NEXT -> {
                List<Long> list = playlist(roomId);
                if (list.isEmpty()) throw new BizException(ErrorCode.ROOM_SONG);
                int index = list.indexOf(state.getCurrentSongId());
                state.setCurrentSongId(list.get((index + 1) % list.size())); state.setPositionMs(0L); state.setStatus("playing");
            }
            default -> throw new BizException(ErrorCode.PARAM);
        }
        state.setServerTimestamp(now);
        savePlayback(roomId, state);
        publisher.broadcast(roomId, type, state);
        return state;
    }
    public synchronized List<Long> updatePlaylist(Long userId, Long roomId, List<Long> songIds) {
        requireController(userId, roomId);
        if (songIds == null || songIds.size() > 500 || songIds.stream().anyMatch(id -> id == null || id < 1) || new LinkedHashSet<>(songIds).size() != songIds.size()) throw new BizException(ErrorCode.PARAM);
        for (Long songId : songIds) songs.require(songId);
        String key = RedisKeys.roomPlaylist(roomId);
        redis.delete(key);
        if (!songIds.isEmpty()) redis.opsForList().rightPushAll(key, songIds.stream().map(String::valueOf).toList());
        redis.expire(key, ACTIVE_TTL);
        PlaybackState state = playback(roomId);
        if (state.getCurrentSongId() != null && !songIds.contains(state.getCurrentSongId())) {
            state.setCurrentSongId(null); state.setStatus("paused"); state.setPositionMs(0L); state.setServerTimestamp(System.currentTimeMillis()); savePlayback(roomId, state);
        }
        publisher.broadcast(roomId, WsTypes.PLAYLIST_UPDATE, songIds);
        return songIds;
    }
    private List<Long> playlist(Long roomId) {
        List<String> raw = redis.opsForList().range(RedisKeys.roomPlaylist(roomId), 0, -1);
        return raw == null ? List.of() : raw.stream().map(Long::valueOf).toList();
    }
    private void savePlayback(Long roomId, PlaybackState state) {
        try { redis.opsForValue().set(RedisKeys.roomState(roomId), json.writeValueAsString(state), ACTIVE_TTL); }
        catch (JsonProcessingException ex) { throw new IllegalStateException("无法序列化房间状态", ex); }
    }
    private void refreshRoom(Long roomId) { expireRoom(roomId, ACTIVE_TTL); }
    private void expireRoom(Long roomId, Duration ttl) {
        redis.expire(RedisKeys.roomState(roomId), ttl);
        redis.expire(RedisKeys.roomMembers(roomId), ttl);
        redis.expire(RedisKeys.roomPlaylist(roomId), ttl);
        redis.expire(RedisKeys.roomControllers(roomId), ttl);
    }
    public void refreshPresence(Long userId) {
        String key = RedisKeys.userRoom(userId);
        String roomId = redis.opsForValue().get(key);
        if (roomId != null) { redis.expire(key, ACTIVE_TTL); refreshRoom(Long.valueOf(roomId)); }
    }
}
