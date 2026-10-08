package com.chengxu.futari.invite.dto;

import com.chengxu.futari.user.dto.UserResponse;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

/** 邀请通知及创建结果。 @author Futari */
@Getter @Setter @AllArgsConstructor @NoArgsConstructor
public class InviteResponse {
    private String inviteId;
    private Long roomId;
    private String roomName;
    private UserResponse from;
    private String currentSongName;
    private Long memberCount;
    private Long expiresAt;
}
