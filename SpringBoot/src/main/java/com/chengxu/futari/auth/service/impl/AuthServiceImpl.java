package com.chengxu.futari.auth.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.chengxu.futari.auth.JwtUtil;
import com.chengxu.futari.auth.dto.*;
import com.chengxu.futari.auth.service.AuthService;
import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.user.entity.User;
import com.chengxu.futari.user.mapper.UserMapper;
import com.chengxu.futari.sync.SyncPublisher;
import io.jsonwebtoken.Claims;
import java.time.Duration;
import java.util.UUID;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;

/** 有状态 JWT 会话实现。 @author Futari */
@Slf4j @Service @RequiredArgsConstructor
public class AuthServiceImpl implements AuthService {
    private final UserMapper users;
    private final StringRedisTemplate redis;
    private final JwtUtil jwt;
    private final PasswordEncoder encoder;
    private final SyncPublisher publisher;
    public synchronized Long register(RegisterRequest request) {
        if (users.selectCount(new LambdaQueryWrapper<User>().eq(User::getUsername, request.getUsername())) > 0) throw new BizException(ErrorCode.USER_EXISTS);
        User user = new User();
        user.setUsername(request.getUsername()); user.setNickname(request.getNickname()); user.setPasswordHash(encoder.encode(request.getPassword()));
        // This deployment runs one backend instance. Serialize bootstrap registration so one account receives ADMIN.
        user.setRole(users.selectCount(new LambdaQueryWrapper<User>()) == 0 ? "ADMIN" : "USER");
        user.setCanUpload(false); user.setCanManageServerPlaylist(false);
        try { users.insert(user); } catch (DuplicateKeyException ex) { throw new BizException(ErrorCode.USER_EXISTS); }
        return user.getId();
    }
    public LoginResponse login(LoginRequest request) {
        User user = users.selectOne(new LambdaQueryWrapper<User>().eq(User::getUsername, request.getUsername()));
        if (user == null || !encoder.matches(request.getPassword(), user.getPasswordHash())) throw new BizException(ErrorCode.BAD_CREDENTIALS);
        String jti = UUID.randomUUID().toString();
        String token = jwt.issue(user.getId(), jti);
        redis.opsForValue().set(RedisKeys.login(user.getId()), jti, Duration.ofSeconds(jwt.ttlSeconds()));
        publisher.close(user.getId());
        log.info("user {} logged in", user.getId());
        return new LoginResponse(token, user.getId(), user.getRole(), user.getCanUpload(), user.getCanManageServerPlaylist());
    }
    public void logout(Long userId) { redis.delete(RedisKeys.login(userId)); publisher.close(userId); log.info("user {} logged out", userId); }
    public Long authenticate(String token) {
        Claims claims = jwt.parse(token);
        Object raw = claims.get("userId");
        if (!(raw instanceof Number) || claims.getId() == null) throw new BizException(ErrorCode.UNAUTHORIZED);
        Long id = ((Number) raw).longValue();
        String stored = redis.opsForValue().get(RedisKeys.login(id));
        if (!claims.getId().equals(stored)) throw new BizException(ErrorCode.UNAUTHORIZED);
        return id;
    }
}
