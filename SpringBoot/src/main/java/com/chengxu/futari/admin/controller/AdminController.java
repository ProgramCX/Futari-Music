package com.chengxu.futari.admin.controller;

import com.chengxu.futari.admin.dto.*;
import com.chengxu.futari.admin.service.AdminService;
import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.common.result.PageResult;
import lombok.RequiredArgsConstructor;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 管理员专用接口。 @author Futari */
@RequirePermission("admin") @RestController @RequestMapping("/api/admin") @RequiredArgsConstructor
public class AdminController {
    private final AdminService service;
    @GetMapping("/users") public ApiResult<PageResult<AdminUserResponse>> list(@RequestParam(defaultValue = "1") int pageNum, @RequestParam(defaultValue = "20") int pageSize) { return ApiResult.ok(service.list(pageNum, pageSize)); }
    @PutMapping("/users/{id}/permissions") public ApiResult<AdminUserResponse> permissions(@PathVariable Long id, @Validated @RequestBody PermissionsUpdateRequest request) { return ApiResult.ok(service.permissions(id, request.getCanUpload(), request.getCanManageServerPlaylist())); }
    @PutMapping("/users/{id}/role") public ApiResult<AdminUserResponse> role(@PathVariable Long id, @Validated @RequestBody RoleUpdateRequest request) { return ApiResult.ok(service.role(id, request.getRole())); }
    @PutMapping("/users/{id}/password") public ApiResult<Void> password(@PathVariable Long id, @Validated @RequestBody PasswordUpdateRequest request) { service.password(id, request.getNewPassword()); return ApiResult.ok(); }
    @PostMapping("/users/{id}/kick") public ApiResult<Void> kick(@PathVariable Long id) { service.kick(id); return ApiResult.ok(); }
}
