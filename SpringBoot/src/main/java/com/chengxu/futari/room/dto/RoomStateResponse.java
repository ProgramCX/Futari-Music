package com.chengxu.futari.room.dto;

import com.chengxu.futari.user.dto.UserResponse;
import java.util.List;
import lombok.AllArgsConstructor;
import lombok.Getter;

/** 进房和断线恢复用的全量状态。 @author Futari */
@Getter @AllArgsConstructor
public class RoomStateResponse {
    private RoomResponse room;
    private PlaybackState playback;
    private List<Long> songIds;
    private List<UserResponse> members;
    private List<Long> controllerIds;
    private Long serverTime;
}
