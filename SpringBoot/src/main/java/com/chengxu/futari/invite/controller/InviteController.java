package com.chengxu.futari.invite.controller;

import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.invite.dto.*;
import com.chengxu.futari.invite.service.InviteService;
import com.chengxu.futari.room.dto.RoomStateResponse;
import lombok.RequiredArgsConstructor;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;
import java.util.List;

/** 邀请接口。 @author Futari */
@RestController @RequiredArgsConstructor
public class InviteController {
    private final InviteService service;
    @GetMapping("/api/invites") public ApiResult<List<InviteResponse>> pending() { return ApiResult.ok(service.pending(UserContext.getUserId())); }
    @PostMapping("/api/rooms/{id}/invite") public ApiResult<InviteResponse> create(@PathVariable Long id, @Validated @RequestBody InviteCreateRequest request) { return ApiResult.ok(service.create(UserContext.getUserId(), id, request.getPartnerId())); }
    @PostMapping("/api/invites/{inviteId}/accept") public ApiResult<RoomStateResponse> accept(@PathVariable String inviteId) { return ApiResult.ok(service.accept(UserContext.getUserId(), inviteId)); }
    @PostMapping("/api/invites/{inviteId}/decline") public ApiResult<Void> decline(@PathVariable String inviteId) { service.decline(UserContext.getUserId(), inviteId); return ApiResult.ok(); }
}
