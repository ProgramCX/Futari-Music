package com.chengxu.futari.auth;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.user.entity.User;
import com.chengxu.futari.user.mapper.UserMapper;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Component;
import lombok.extern.slf4j.Slf4j;

/** 使用本机配置在首次启动时创建管理员账号。 @author Futari */
@Component
@Slf4j
public class BootstrapAdminRunner implements ApplicationRunner {
    private final UserMapper users;
    private final PasswordEncoder passwordEncoder;
    private final StringRedisTemplate redis;
    private final String username;
    private final String nickname;
    private final String password;
    private final boolean resetExisting;

    public BootstrapAdminRunner(
            UserMapper users,
            PasswordEncoder passwordEncoder,
            StringRedisTemplate redis,
            @Value("${futari.bootstrap-admin.username:admin}") String username,
            @Value("${futari.bootstrap-admin.nickname:管理员}") String nickname,
            @Value("${futari.bootstrap-admin.password:}") String password,
            @Value("${futari.bootstrap-admin.reset-existing:false}") boolean resetExisting) {
        this.users = users;
        this.passwordEncoder = passwordEncoder;
        this.redis = redis;
        this.username = username;
        this.nickname = nickname;
        this.password = password;
        this.resetExisting = resetExisting;
    }

    @Override
    public synchronized void run(ApplicationArguments args) {
        if (password.isBlank()) return;
        if (password.length() < 12 || password.length() > 72) {
            throw new IllegalStateException("BOOTSTRAP_ADMIN_PASSWORD 长度必须为 12 到 72 个字符");
        }
        User existing = users.selectOne(new LambdaQueryWrapper<User>().eq(User::getUsername, username));
        if (existing != null) {
            if (!resetExisting) {
                if ("ADMIN".equals(existing.getRole())) return;
                throw new IllegalStateException("引导管理员用户名已被普通用户占用，请更换 BOOTSTRAP_ADMIN_USERNAME");
            }
            existing.setPasswordHash(passwordEncoder.encode(password));
            existing.setRole("ADMIN");
            users.updateById(existing);
            redis.delete(RedisKeys.login(existing.getId()));
            log.info("已重置引导管理员账号 {} 的密码并授予管理员权限", username);
            return;
        }

        User admin = new User();
        admin.setUsername(username);
        admin.setNickname(nickname);
        admin.setPasswordHash(passwordEncoder.encode(password));
        admin.setRole("ADMIN");
        admin.setCanUpload(false);
        admin.setCanManageServerPlaylist(false);
        users.insert(admin);
        log.info("已创建引导管理员账号 {}", username);
    }
}
