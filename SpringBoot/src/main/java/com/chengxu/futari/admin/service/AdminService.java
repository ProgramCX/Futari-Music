package com.chengxu.futari.admin.service;

import com.chengxu.futari.admin.dto.AdminUserResponse;
import com.chengxu.futari.common.result.PageResult;

/** 管理用户权限、角色和会话。 @author Futari */
public interface AdminService {
    PageResult<AdminUserResponse> list(int pageNum, int pageSize);
    AdminUserResponse permissions(Long id, boolean canUpload, boolean canManageServerPlaylist);
    AdminUserResponse role(Long id, String role);
    void password(Long id, String newPassword);
    void kick(Long id);
}
