package com.chengxu.futari.admin.service.impl;

import com.chengxu.futari.admin.dto.AdminUserResponse;
import com.chengxu.futari.admin.service.AdminService;
import com.chengxu.futari.auth.service.AuthService;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.sync.SyncPublisher;
import com.chengxu.futari.user.entity.User;
import com.chengxu.futari.user.service.UserService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;

/** 管理端业务实现。 @author Futari */
@Slf4j @Service @RequiredArgsConstructor
public class AdminServiceImpl implements AdminService {
    private final UserService users;
    private final AuthService auth;
    private final SyncPublisher publisher;
    private final PasswordEncoder encoder;
    private AdminUserResponse response(User user) { return new AdminUserResponse(user.getId(), user.getUsername(), user.getNickname(), user.getRole(), user.getCanUpload(), user.getCanManageServerPlaylist()); }
    public PageResult<AdminUserResponse> list(int pageNum, int pageSize) {
        PageResult<User> page = users.adminList(pageNum, pageSize);
        return new PageResult<>(page.getTotal(), page.getList().stream().map(this::response).toList());
    }
    public AdminUserResponse permissions(Long id, boolean canUpload, boolean canManageServerPlaylist) { return response(users.updatePermissions(id, canUpload, canManageServerPlaylist)); }
    public AdminUserResponse role(Long id, String role) { return response(users.updateRole(id, role)); }
    public void password(Long id, String newPassword) { users.updatePassword(id, encoder.encode(newPassword)); kick(id); }
    public void kick(Long id) {
        users.require(id);
        publisher.send(id, WsTypes.KICKED, "管理员已结束当前会话");
        auth.logout(id);
        log.info("admin kicked user {}", id);
    }
}
