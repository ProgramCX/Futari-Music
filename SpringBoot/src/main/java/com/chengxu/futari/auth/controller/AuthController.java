package com.chengxu.futari.auth.controller;

import com.chengxu.futari.auth.dto.*;
import com.chengxu.futari.auth.service.AuthService;
import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.user.service.UserService;
import lombok.RequiredArgsConstructor;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 注册登录接口。 @author Futari */
@RestController @RequestMapping("/api/auth") @RequiredArgsConstructor
public class AuthController {
    private final AuthService service;
    private final UserService users;
    @PostMapping("/register") public ApiResult<Long> register(@Validated @RequestBody RegisterRequest request) { return ApiResult.ok(service.register(request)); }
    @PostMapping("/login") public ApiResult<LoginResponse> login(@Validated @RequestBody LoginRequest request) { return ApiResult.ok(service.login(request)); }
    @GetMapping("/me") public ApiResult<SessionResponse> me() {
        var user = users.require(UserContext.getUserId());
        return ApiResult.ok(new SessionResponse(user.getId(), user.getUsername(), user.getRole(),
                user.getCanUpload(), user.getCanManageServerPlaylist()));
    }
    @PostMapping("/logout") public ApiResult<Void> logout() { service.logout(UserContext.getUserId()); return ApiResult.ok(); }
}
