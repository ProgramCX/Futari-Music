package com.chengxu.futari.sync;

import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.room.service.RoomService;
import java.util.HashSet;
import java.util.Set;
import lombok.RequiredArgsConstructor;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

/** 活跃房间定期同步播放基准。 @author Futari */
@Component @EnableScheduling @RequiredArgsConstructor
public class SyncScheduler {
    private final SyncPublisher publisher;
    private final RoomService rooms;
    private final StringRedisTemplate redis;
    @Scheduled(fixedDelay = 15000)
    public void sync() {
        Set<Long> roomIds = new HashSet<>();
        for (Long userId : publisher.onlineUserIds()) {
            String room = redis.opsForValue().get(RedisKeys.userRoom(userId));
            if (room != null) roomIds.add(Long.valueOf(room));
        }
        for (Long roomId : roomIds) publisher.broadcast(roomId, WsTypes.SYNC, rooms.playback(roomId));
    }
}
