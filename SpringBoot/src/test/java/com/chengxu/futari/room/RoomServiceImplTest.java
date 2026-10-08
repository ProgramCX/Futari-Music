package com.chengxu.futari.room;

import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.partner.service.PartnerService;
import com.chengxu.futari.room.mapper.RoomMapper;
import com.chengxu.futari.room.entity.Room;
import com.chengxu.futari.room.dto.RoomDeletedResponse;
import com.chengxu.futari.room.service.impl.RoomServiceImpl;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.sync.SyncPublisher;
import com.chengxu.futari.user.service.UserService;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.data.redis.core.script.DefaultRedisScript;
import java.util.List;
import java.util.Set;
import org.mockito.ArgumentCaptor;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

/** 验证非成员不能改变共享播放状态。 @author Futari */
class RoomServiceImplTest {
    @Test void outsiderCannotPlay() {
        StringRedisTemplate redis = mock(StringRedisTemplate.class, RETURNS_DEEP_STUBS);
        SyncPublisher publisher = mock(SyncPublisher.class);
        RoomServiceImpl service = new RoomServiceImpl(mock(RoomMapper.class), redis, new ObjectMapper(), mock(UserService.class), mock(SongService.class), publisher, mock(PartnerService.class));
        when(redis.opsForSet().isMember(RedisKeys.roomMembers(9L), "3")).thenReturn(false);
        BizException ex = assertThrows(BizException.class, () -> service.control(3L, 9L, WsTypes.PLAY, 8L, 0L));
        assertEquals(ErrorCode.ROOM_NOT_MEMBER, ex.getErrorCode());
        verifyNoInteractions(publisher);
    }
    @Test void staleCurrentRoomMappingIsRemoved() {
        StringRedisTemplate redis = mock(StringRedisTemplate.class, RETURNS_DEEP_STUBS);
        RoomServiceImpl service = new RoomServiceImpl(mock(RoomMapper.class), redis, new ObjectMapper(),
                mock(UserService.class), mock(SongService.class), mock(SyncPublisher.class), mock(PartnerService.class));
        when(redis.opsForValue().get(RedisKeys.userRoom(3L))).thenReturn("9");
        when(redis.opsForSet().isMember(RedisKeys.roomMembers(9L), "3")).thenReturn(false);

        assertNull(service.current(3L));
        verify(redis).execute(any(DefaultRedisScript.class), eq(List.of(RedisKeys.userRoom(3L))), eq("9"));
    }
    @Test void creatorCanDissolveRoomAfterLeavingAndMembersAreNotLoggedOut() {
        RoomMapper mapper = mock(RoomMapper.class);
        Room room = new Room(); room.setId(9L); room.setOwnerId(1L);
        when(mapper.selectById(9L)).thenReturn(room);
        when(mapper.deleteById(9L)).thenReturn(1);
        StringRedisTemplate redis = mock(StringRedisTemplate.class, RETURNS_DEEP_STUBS);
        when(redis.opsForSet().members(RedisKeys.roomMembers(9L))).thenReturn(Set.of("2", "3"));
        SyncPublisher publisher = mock(SyncPublisher.class);
        PartnerService partners = mock(PartnerService.class);
        RoomServiceImpl service = new RoomServiceImpl(mapper, redis, new ObjectMapper(), mock(UserService.class), mock(SongService.class), publisher, partners);

        service.delete(1L, 9L);

        ArgumentCaptor<RoomDeletedResponse> notification = ArgumentCaptor.forClass(RoomDeletedResponse.class);
        verify(publisher).broadcast(eq(9L), eq(WsTypes.ROOM_DELETED), notification.capture());
        assertEquals(9L, notification.getValue().getRoomId());
        for (long member : List.of(1L, 2L, 3L)) {
            verify(redis).execute(any(DefaultRedisScript.class), eq(List.of(RedisKeys.userRoom(member))), eq("9"));
            verify(partners).publishStatus(member);
        }
        verify(redis).delete(List.of(RedisKeys.roomState(9L), RedisKeys.roomMembers(9L), RedisKeys.roomPlaylist(9L), RedisKeys.roomControllers(9L)));
        verify(publisher, never()).close(anyLong());
    }
    @Test void otherMemberAndAdministratorCannotDissolveAnotherUsersRoom() {
        RoomMapper mapper = mock(RoomMapper.class);
        Room room = new Room(); room.setId(9L); room.setOwnerId(1L);
        when(mapper.selectById(9L)).thenReturn(room);
        StringRedisTemplate redis = mock(StringRedisTemplate.class);
        SyncPublisher publisher = mock(SyncPublisher.class);
        RoomServiceImpl service = new RoomServiceImpl(mapper, redis, new ObjectMapper(), mock(UserService.class), mock(SongService.class), publisher, mock(PartnerService.class));

        BizException exception = assertThrows(BizException.class, () -> service.delete(2L, 9L));

        assertEquals(ErrorCode.ROOM_OWNER_ONLY, exception.getErrorCode());
        verify(mapper, never()).deleteById(anyLong());
        verifyNoInteractions(redis, publisher);
    }
}
