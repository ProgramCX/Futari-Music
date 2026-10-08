package com.chengxu.futari.user.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.chengxu.futari.common.constant.RedisKeys;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.user.dto.UserResponse;
import com.chengxu.futari.user.entity.User;
import com.chengxu.futari.user.mapper.UserMapper;
import com.chengxu.futari.user.service.UserService;
import java.util.List;
import lombok.RequiredArgsConstructor;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Service;

/** 用户查询及集中权限判断。 @author Futari */
@Service @RequiredArgsConstructor
public class UserServiceImpl implements UserService {
    private final UserMapper mapper;
    private final StringRedisTemplate redis;
    public User require(Long id) {
        User user = mapper.selectById(id);
        if (user == null) throw new BizException(ErrorCode.USER_NOT_FOUND);
        return user;
    }
    public UserResponse response(User user) {
        String room = redis.opsForValue().get(RedisKeys.userRoom(user.getId()));
        return new UserResponse(user.getId(), user.getNickname(), user.getAvatarUrl(), Boolean.TRUE.equals(redis.hasKey(RedisKeys.online(user.getId()))), room == null ? null : Long.valueOf(room));
    }
    public PageResult<UserResponse> search(String keyword, int pageNum, int pageSize) {
        if (pageNum < 1 || pageSize < 1 || pageSize > 100) throw new BizException(ErrorCode.PARAM);
        LambdaQueryWrapper<User> query = new LambdaQueryWrapper<User>().like(keyword != null && !keyword.isBlank(), User::getNickname, keyword == null ? "" : keyword.trim()).orderByAsc(User::getId);
        Page<User> page = mapper.selectPage(new Page<>(pageNum, pageSize), query);
        List<UserResponse> list = page.getRecords().stream().map(this::response).toList();
        return new PageResult<>(page.getTotal(), list);
    }
    public boolean hasPermission(Long userId, String permission) {
        User user = require(userId);
        if ("ADMIN".equals(user.getRole())) return true;
        return switch (permission) {
            case "upload" -> Boolean.TRUE.equals(user.getCanUpload());
            case "serverPlaylist" -> Boolean.TRUE.equals(user.getCanManageServerPlaylist());
            default -> false;
        };
    }
    public PageResult<User> adminList(int pageNum, int pageSize) {
        if (pageNum < 1 || pageSize < 1 || pageSize > 100) throw new BizException(ErrorCode.PARAM);
        Page<User> page = mapper.selectPage(new Page<>(pageNum, pageSize), new LambdaQueryWrapper<User>().orderByDesc(User::getId));
        return new PageResult<>(page.getTotal(), page.getRecords());
    }
    public User updatePermissions(Long id, boolean canUpload, boolean canManageServerPlaylist) {
        User user = require(id); user.setCanUpload(canUpload); user.setCanManageServerPlaylist(canManageServerPlaylist); mapper.updateById(user); return user;
    }
    public synchronized User updateRole(Long id, String role) {
        if (!"USER".equals(role) && !"ADMIN".equals(role)) throw new BizException(ErrorCode.PARAM);
        User user = require(id);
        if ("ADMIN".equals(user.getRole()) && "USER".equals(role) && mapper.selectCount(new LambdaQueryWrapper<User>().eq(User::getRole, "ADMIN")) <= 1) throw new BizException(ErrorCode.ADMIN_LAST);
        user.setRole(role); mapper.updateById(user); return user;
    }
    public void updatePassword(Long id, String hash) { User user = require(id); user.setPasswordHash(hash); mapper.updateById(user); }
}
