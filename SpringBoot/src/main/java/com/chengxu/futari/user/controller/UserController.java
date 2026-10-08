package com.chengxu.futari.user.controller;

import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.user.dto.UserResponse;
import com.chengxu.futari.user.service.UserService;
import lombok.RequiredArgsConstructor;
import org.springframework.web.bind.annotation.*;

/** 用户搜索接口。 @author Futari */
@RestController @RequestMapping("/api/users") @RequiredArgsConstructor
public class UserController {
    private final UserService service;
    @GetMapping("/search")
    public ApiResult<PageResult<UserResponse>> search(@RequestParam(defaultValue = "") String keyword, @RequestParam(defaultValue = "1") int pageNum, @RequestParam(defaultValue = "20") int pageSize) {
        return ApiResult.ok(service.search(keyword, pageNum, pageSize));
    }
}
