package com.chengxu.futari.user.service;

import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.user.dto.UserResponse;
import com.chengxu.futari.user.entity.User;
import java.util.List;

/** 用户资料与权限查询。 @author Futari */
public interface UserService {
    User require(Long id);
    UserResponse response(User user);
    PageResult<UserResponse> search(String keyword, int pageNum, int pageSize);
    boolean hasPermission(Long userId, String permission);
    PageResult<User> adminList(int pageNum, int pageSize);
    User updatePermissions(Long id, boolean canUpload, boolean canManageServerPlaylist);
    User updateRole(Long id, String role);
    void updatePassword(Long id, String hash);
}
