package com.chengxu.futari.room.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 房间摘要。 @author Futari */
@Getter @AllArgsConstructor
public class RoomResponse {
    private Long id;
    private String name;
    private Long ownerId;
    private Long memberCount;
    private Long createdAt;
}
