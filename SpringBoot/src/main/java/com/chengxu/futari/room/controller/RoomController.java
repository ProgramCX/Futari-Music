package com.chengxu.futari.room.controller;

import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.room.dto.*;
import com.chengxu.futari.room.service.RoomService;
import lombok.RequiredArgsConstructor;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 房间 REST 接口。 @author Futari */
@RestController @RequestMapping("/api/rooms") @RequiredArgsConstructor
public class RoomController {
    private final RoomService service;
    @GetMapping public ApiResult<PageResult<RoomResponse>> list(@RequestParam(defaultValue = "1") int pageNum, @RequestParam(defaultValue = "20") int pageSize) { return ApiResult.ok(service.list(pageNum, pageSize)); }
    @GetMapping("/current") public ApiResult<RoomStateResponse> current() { return ApiResult.ok(service.current(UserContext.getUserId())); }
    @PostMapping public ApiResult<RoomResponse> create(@Validated @RequestBody RoomCreateRequest request) { return ApiResult.ok(service.create(UserContext.getUserId(), request)); }
    @GetMapping("/{id}/state") public ApiResult<RoomStateResponse> state(@PathVariable Long id) { return ApiResult.ok(service.state(UserContext.getUserId(), id)); }
    @PostMapping("/{id}/join") public ApiResult<RoomStateResponse> join(@PathVariable Long id) { return ApiResult.ok(service.join(UserContext.getUserId(), id)); }
    @PostMapping("/{id}/leave") public ApiResult<Void> leave(@PathVariable Long id) { service.leave(UserContext.getUserId(), id); return ApiResult.ok(); }
    @DeleteMapping("/{id}") public ApiResult<Void> delete(@PathVariable Long id) { service.delete(UserContext.getUserId(), id); return ApiResult.ok(); }
    @PutMapping("/{id}/controllers") public ApiResult<Void> control(@PathVariable Long id, @Validated @RequestBody ControlUpdateRequest request) { service.grantControl(UserContext.getUserId(), id, request); return ApiResult.ok(); }
}
