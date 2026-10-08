package com.chengxu.futari.invite;

import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.invite.service.impl.InviteServiceImpl;
import com.chengxu.futari.room.service.RoomService;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.sync.SyncPublisher;
import com.chengxu.futari.user.service.UserService;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.util.Set;
import org.junit.jupiter.api.Test;
import org.springframework.data.redis.core.StringRedisTemplate;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

/** 邀请收件箱只返回目标用户的有效邀请，并清理过期索引。 @author Futari */
class InviteServiceImplTest {
    @Test void pendingFiltersTargetAndCleansExpiredIds() {
        StringRedisTemplate redis = mock(StringRedisTemplate.class, RETURNS_DEEP_STUBS);
        String validId = "11111111-1111-1111-1111-111111111111";
        String expiredId = "22222222-2222-2222-2222-222222222222";
        String foreignId = "33333333-3333-3333-3333-333333333333";
        when(redis.opsForSet().members(RedisKeys.userInvites(7L)))
                .thenReturn(Set.of(validId, expiredId, foreignId));
        when(redis.opsForValue().get(RedisKeys.invite(validId))).thenReturn(
                "{\"targetId\":7,\"response\":{\"inviteId\":\"" + validId
                + "\",\"roomId\":9,\"roomName\":\"一起听\",\"from\":null,\"currentSongName\":null,\"memberCount\":2,\"expiresAt\":4102444800000}}"
        );
        when(redis.opsForValue().get(RedisKeys.invite(foreignId))).thenReturn(
                "{\"targetId\":8,\"response\":{\"inviteId\":\"" + foreignId
                + "\",\"roomId\":9,\"roomName\":\"其他\",\"from\":null,\"currentSongName\":null,\"memberCount\":2,\"expiresAt\":4102444800000}}"
        );
        InviteServiceImpl service = new InviteServiceImpl(redis, mock(RoomService.class),
                mock(UserService.class), mock(SongService.class), mock(SyncPublisher.class), new ObjectMapper());

        var pending = service.pending(7L);

        assertEquals(1, pending.size());
        assertEquals(validId, pending.get(0).getInviteId());
        verify(redis.opsForSet()).remove(RedisKeys.userInvites(7L), expiredId);
    }
}
