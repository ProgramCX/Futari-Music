package com.chengxu.futari.room.dto;

import jakarta.validation.constraints.NotNull;
import lombok.Getter;
import lombok.Setter;

/** 房主授权或撤销播放控制权。 @author Futari */
@Getter @Setter
public class ControlUpdateRequest {
    @NotNull private Long memberId;
    @NotNull private Boolean canControl;
}
