package com.chengxu.futari.admin.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 管理端用户摘要，不含密码。 @author Futari */
@Getter @AllArgsConstructor
public class AdminUserResponse {
    private Long id;
    private String username;
    private String nickname;
    private String role;
    private Boolean canUpload;
    private Boolean canManageServerPlaylist;
}
