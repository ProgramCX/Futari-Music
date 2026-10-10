package com.chengxu.futari.room;

import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.room.dto.PlaybackMode;
import com.chengxu.futari.room.dto.PlaybackState;
import com.chengxu.futari.room.entity.Room;
import com.chengxu.futari.room.mapper.RoomMapper;
import com.chengxu.futari.room.service.impl.RoomServiceImpl;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.user.service.UserService;
import com.chengxu.futari.partner.service.PartnerService;
import com.chengxu.futari.sync.SyncPublisher;
import com.fasterxml.jackson.databind.ObjectMapper;
import java.time.Duration;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.data.redis.core.StringRedisTemplate;
import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

/** 播放结束去重、共享模式与相对队列移动的回归测试。 @author Futari */
class PlaybackOrderTest {
    private final ObjectMapper json = new ObjectMapper();
    private final StringRedisTemplate redis = mock(StringRedisTemplate.class, RETURNS_DEEP_STUBS);
    private final SyncPublisher publisher = mock(SyncPublisher.class);
    private final AtomicReference<String> stored = new AtomicReference<>();
    private RoomServiceImpl service;

    @BeforeEach void setup() throws Exception {
        RoomMapper mapper = mock(RoomMapper.class);
        Room room = new Room(); room.setId(9L); room.setOwnerId(1L);
        when(mapper.selectById(9L)).thenReturn(room);
        when(redis.opsForSet().isMember(RedisKeys.roomMembers(9L), "1")).thenReturn(true);
        when(redis.opsForList().range(RedisKeys.roomPlaylist(9L), 0, -1)).thenReturn(List.of("11", "22", "33"));
        PlaybackState state = new PlaybackState(); state.setCurrentSongId(22L);
        state.setStatus("playing"); state.setServerTimestamp(100L); state.setPositionMs(3000L);
        stored.set(json.writeValueAsString(state));
        when(redis.opsForValue().get(RedisKeys.roomState(9L))).thenAnswer(inv -> stored.get());
        var values = redis.opsForValue();
        doAnswer(inv -> { stored.set(inv.getArgument(1)); return null; }).when(values)
                .set(eq(RedisKeys.roomState(9L)), anyString(), any(Duration.class));
        service = new RoomServiceImpl(mapper, redis, json, mock(UserService.class), mock(SongService.class),
                publisher, mock(PartnerService.class));
    }

    @Test void modePreservesClockAndRequiresController() {
        var result = service.setPlaybackMode(1L, 9L, PlaybackMode.REPEAT_ONE);
        assertEquals(100L, result.getServerTimestamp());
        assertEquals(3000L, result.getPositionMs());
        assertEquals(1L, result.getRevision());
        verify(publisher).broadcast(eq(9L), eq(WsTypes.PLAYBACK_MODE), any(PlaybackState.class));
        assertThrows(BizException.class, () -> service.setPlaybackMode(2L, 9L, PlaybackMode.SHUFFLE));
    }

    @Test void repeatEndRestartsButManualNextAdvances() {
        service.setPlaybackMode(1L, 9L, PlaybackMode.REPEAT_ONE);
        service.trackEnded(1L, 9L, 22L, 100L);
        var repeated = service.playback(9L);
        assertEquals(22L, repeated.getCurrentSongId());
        assertEquals(0L, repeated.getPositionMs());
        service.trackEnded(1L, 9L, 22L, 100L);
        assertEquals(repeated.getServerTimestamp(), service.playback(9L).getServerTimestamp());
        assertEquals(33L, service.control(1L, 9L, WsTypes.NEXT, null, null).getCurrentSongId());
    }

    @Test void sequentialEndStopsAtLastAndStaleEndCannotSkipSong() {
        service.trackEnded(1L, 9L, 22L, 99L);
        assertEquals(22L, service.playback(9L).getCurrentSongId());
        service.trackEnded(1L, 9L, 22L, 100L);
        var last = service.playback(9L);
        assertEquals(33L, last.getCurrentSongId());
        service.trackEnded(1L, 9L, 33L, last.getServerTimestamp());
        assertEquals("paused", service.playback(9L).getStatus());
        assertEquals(33L, service.playback(9L).getCurrentSongId());
    }

    @Test void shuffleAvoidsCurrentSong() {
        service.setPlaybackMode(1L, 9L, PlaybackMode.SHUFFLE);
        for (int i = 0; i < 30; ++i) {
            var previous = service.playback(9L).getCurrentSongId();
            var next = service.control(1L, 9L, WsTypes.NEXT, null, null).getCurrentSongId();
            assertNotEquals(previous, next);
            assertTrue(List.of(11L, 22L, 33L).contains(next));
        }
    }

    @Test void relativeMovePreservesOtherSongsAndCurrentPlayback() {
        service.moveQueueSong(1L, 9L, 33L, 11L);
        verify(redis.opsForList()).rightPushAll(RedisKeys.roomPlaylist(9L), List.of("33", "11", "22"));
        assertEquals(22L, service.playback(9L).getCurrentSongId());
        assertThrows(BizException.class, () -> service.moveQueueSong(1L, 9L, 33L, 999L));
        assertThrows(BizException.class, () -> service.moveQueueSong(2L, 9L, 33L, 11L));
    }
}
