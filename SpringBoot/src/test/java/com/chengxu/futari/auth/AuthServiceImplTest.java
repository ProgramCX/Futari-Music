package com.chengxu.futari.auth;

import com.chengxu.futari.auth.service.impl.AuthServiceImpl;
import com.chengxu.futari.auth.dto.RegisterRequest;
import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.user.mapper.UserMapper;
import com.chengxu.futari.user.entity.User;
import com.baomidou.mybatisplus.core.conditions.Wrapper;
import com.chengxu.futari.sync.SyncPublisher;
import org.junit.jupiter.api.Test;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.security.crypto.password.PasswordEncoder;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;
import static org.mockito.ArgumentMatchers.any;
import java.util.concurrent.atomic.AtomicInteger;

/** 验证签名之外的 Redis 会话撤销规则。 @author Futari */
class AuthServiceImplTest {
    @Test void firstRegisteredAccountBecomesAdminAndFollowingAccountIsUser() {
        UserMapper users = mock(UserMapper.class);
        AtomicInteger queries = new AtomicInteger();
        long[] results = {0L, 0L, 0L, 1L};
        when(users.selectCount(any(Wrapper.class))).thenAnswer(invocation -> results[queries.getAndIncrement()]);
        PasswordEncoder encoder = mock(PasswordEncoder.class);
        when(encoder.encode(anyString())).thenReturn("encoded");
        AuthServiceImpl service = new AuthServiceImpl(users, mock(StringRedisTemplate.class),
                new JwtUtil("01234567890123456789012345678901", 3600), encoder, mock(SyncPublisher.class));

        service.register(registerRequest("first"));
        service.register(registerRequest("second"));

        var inserted = org.mockito.ArgumentCaptor.forClass(User.class);
        verify(users, times(2)).insert(inserted.capture());
        assertEquals("ADMIN", inserted.getAllValues().get(0).getRole());
        assertEquals("USER", inserted.getAllValues().get(1).getRole());
    }

    private RegisterRequest registerRequest(String username) {
        RegisterRequest request = new RegisterRequest();
        request.setUsername(username);
        request.setNickname(username);
        request.setPassword("password123");
        return request;
    }

    @Test void revokedTokenCannotAuthenticate() {
        StringRedisTemplate redis = mock(StringRedisTemplate.class, RETURNS_DEEP_STUBS);
        JwtUtil jwt = new JwtUtil("01234567890123456789012345678901", 3600);
        AuthServiceImpl service = new AuthServiceImpl(mock(UserMapper.class), redis, jwt, mock(PasswordEncoder.class), mock(SyncPublisher.class));
        String token = jwt.issue(42L, "session-a");
        when(redis.opsForValue().get(RedisKeys.login(42L))).thenReturn("session-b");
        BizException ex = assertThrows(BizException.class, () -> service.authenticate(token));
        assertEquals(ErrorCode.UNAUTHORIZED, ex.getErrorCode());
    }
}
