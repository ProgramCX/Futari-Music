package com.chengxu.futari.room.service;

import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.room.dto.*;
import com.chengxu.futari.room.entity.Room;
import java.util.List;

/** 房间成员、授权与共享播放状态。 @author Futari */
public interface RoomService {
    Room require(Long roomId);
    RoomResponse create(Long userId, RoomCreateRequest request);
    PageResult<RoomResponse> list(int pageNum, int pageSize);
    RoomStateResponse state(Long userId, Long roomId);
    RoomStateResponse current(Long userId);
    RoomStateResponse join(Long userId, Long roomId);
    void leave(Long userId, Long roomId);
    void delete(Long userId, Long roomId);
    void grantControl(Long ownerId, Long roomId, ControlUpdateRequest request);
    void requireMember(Long userId, Long roomId);
    void requireController(Long userId, Long roomId);
    PlaybackState playback(Long roomId);
    PlaybackState setPlaybackMode(Long userId, Long roomId, PlaybackMode mode);
    void trackEnded(Long userId, Long roomId, Long songId, Long timestamp);
    PlaybackState control(Long userId, Long roomId, String type, Long songId, Long positionMs);
    List<Long> updatePlaylist(Long userId, Long roomId, List<Long> songIds);
    void refreshPresence(Long userId);
    void moveQueueSong(Long userId, Long roomId, Long songId, Long beforeId);
}
