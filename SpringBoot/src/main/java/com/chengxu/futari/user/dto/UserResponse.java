package com.chengxu.futari.user.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 对外公开的用户资料。 @author Futari */
@Getter @AllArgsConstructor
public class UserResponse {
    private Long id;
    private String nickname;
    private String avatarUrl;
    private Boolean online;
    private Long roomId;
}
