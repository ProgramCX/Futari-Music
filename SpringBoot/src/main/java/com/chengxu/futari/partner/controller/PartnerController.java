package com.chengxu.futari.partner.controller;

import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.partner.dto.*;
import com.chengxu.futari.partner.service.PartnerService;
import java.util.List;
import lombok.RequiredArgsConstructor;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 常听搭子接口。 @author Futari */
@RestController @RequestMapping("/api/partners") @RequiredArgsConstructor
public class PartnerController {
    private final PartnerService service;
    @GetMapping public ApiResult<List<PartnerResponse>> list() { return ApiResult.ok(service.list(UserContext.getUserId())); }
    @PostMapping("/{userId}") public ApiResult<PartnerResponse> add(@PathVariable Long userId, @Validated @RequestBody(required = false) PartnerCreateRequest request) { return ApiResult.ok(service.add(UserContext.getUserId(), userId, request == null ? null : request.getRemark())); }
    @DeleteMapping("/{userId}") public ApiResult<Void> remove(@PathVariable Long userId) { service.remove(UserContext.getUserId(), userId); return ApiResult.ok(); }
}
