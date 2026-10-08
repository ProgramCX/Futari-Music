package com.chengxu.futari.auth.service;

import com.chengxu.futari.auth.dto.*;

/** 注册登录与会话校验。 @author Futari */
public interface AuthService {
    Long register(RegisterRequest request);
    LoginResponse login(LoginRequest request);
    void logout(Long userId);
    Long authenticate(String token);
}
