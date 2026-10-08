package com.chengxu.futari.room.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 房间解散通知，客户端只清理对应房间。 @author Futari */
@Getter @AllArgsConstructor
public class RoomDeletedResponse {
    private Long roomId;
}
