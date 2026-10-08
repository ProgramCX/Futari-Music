package com.chengxu.futari.auth;

import com.baomidou.mybatisplus.core.conditions.Wrapper;
import com.chengxu.futari.user.entity.User;
import com.chengxu.futari.user.mapper.UserMapper;
import com.chengxu.futari.common.constant.RedisKeys;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.junit.jupiter.api.Test;
import org.springframework.security.crypto.password.PasswordEncoder;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.*;

class BootstrapAdminRunnerTest {
    @Test void createsConfiguredAdminWhenNoAdminExists() {
        UserMapper users = mock(UserMapper.class);
        when(users.selectCount(any(Wrapper.class))).thenReturn(0L);
        when(users.selectOne(any(Wrapper.class))).thenReturn(null);
        PasswordEncoder encoder = mock(PasswordEncoder.class);
        when(encoder.encode("local-strong-password")).thenReturn("bcrypt-hash");

        new BootstrapAdminRunner(users, encoder, mock(StringRedisTemplate.class),
                "admin", "管理员", "local-strong-password", false).run(null);

        var inserted = org.mockito.ArgumentCaptor.forClass(User.class);
        verify(users).insert(inserted.capture());
        assertEquals("admin", inserted.getValue().getUsername());
        assertEquals("ADMIN", inserted.getValue().getRole());
        assertEquals("bcrypt-hash", inserted.getValue().getPasswordHash());
    }

    @Test void leavesExistingAdminAlone() {
        UserMapper users = mock(UserMapper.class);
        User existing = new User();
        existing.setRole("ADMIN");
        when(users.selectOne(any(Wrapper.class))).thenReturn(existing);
        PasswordEncoder encoder = mock(PasswordEncoder.class);

        new BootstrapAdminRunner(users, encoder, mock(StringRedisTemplate.class),
                "admin", "管理员", "local-strong-password", false).run(null);

        verify(users, never()).insert(any(User.class));
        verifyNoInteractions(encoder);
    }

    @Test void rejectsBootstrapUsernameAlreadyOwnedByRegularUser() {
        UserMapper users = mock(UserMapper.class);
        when(users.selectOne(any(Wrapper.class))).thenReturn(new User());
        BootstrapAdminRunner runner = new BootstrapAdminRunner(users, mock(PasswordEncoder.class), mock(StringRedisTemplate.class),
                "admin", "管理员", "local-strong-password", false);

        assertThrows(IllegalStateException.class, () -> runner.run(null));
        verify(users, never()).insert(any(User.class));
    }

    @Test void requiresStrongConfiguredPassword() {
        BootstrapAdminRunner runner = new BootstrapAdminRunner(mock(UserMapper.class), mock(PasswordEncoder.class), mock(StringRedisTemplate.class),
                "admin", "管理员", "short", false);

        assertThrows(IllegalStateException.class, () -> runner.run(null));
    }

    @Test void canResetConfiguredExistingAdminOnExplicitOneShotRequest() {
        UserMapper users = mock(UserMapper.class);
        User existing = new User();
        existing.setId(42L);
        existing.setRole("ADMIN");
        when(users.selectOne(any(Wrapper.class))).thenReturn(existing);
        PasswordEncoder encoder = mock(PasswordEncoder.class);
        when(encoder.encode("local-strong-password")).thenReturn("new-bcrypt-hash");
        StringRedisTemplate redis = mock(StringRedisTemplate.class);

        new BootstrapAdminRunner(users, encoder, redis,
                "admin", "管理员", "local-strong-password", true).run(null);

        verify(users).updateById(existing);
        verify(redis).delete(RedisKeys.login(42L));
        assertEquals("ADMIN", existing.getRole());
        assertEquals("new-bcrypt-hash", existing.getPasswordHash());
    }
}
