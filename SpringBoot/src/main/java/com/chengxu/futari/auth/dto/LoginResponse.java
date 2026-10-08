package com.chengxu.futari.auth.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 登录结果，仅登录接口可包含令牌。 @author Futari */
@Getter @AllArgsConstructor
public class LoginResponse {
    private String token;
    private Long userId;
    private String role;
    private Boolean canUpload;
    private Boolean canManageServerPlaylist;
}
