package com.chengxu.futari.auth.dto;

/** 已认证会话的当前账号与权限，不包含令牌或密码。 @author Futari */
public record SessionResponse(Long userId, String username, String role,
                              Boolean canUpload, Boolean canManageServerPlaylist) { }
