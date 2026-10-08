package com.chengxu.futari.playlist.personal.controller;

import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.playlist.personal.dto.*;
import com.chengxu.futari.playlist.personal.service.UserPlaylistService;
import java.util.List;
import lombok.RequiredArgsConstructor;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 个人歌单接口。 @author Futari */
@RestController @RequestMapping("/api/playlists") @RequiredArgsConstructor
public class UserPlaylistController {
    private final UserPlaylistService service;
    @GetMapping public ApiResult<List<PlaylistResponse>> list() { return ApiResult.ok(service.list(UserContext.getUserId())); }
    @GetMapping("/{id}") public ApiResult<PlaylistResponse> get(@PathVariable Long id) { return ApiResult.ok(service.get(UserContext.getUserId(), id)); }
    @PostMapping public ApiResult<PlaylistResponse> create(@Validated @RequestBody PlaylistCreateRequest request) { return ApiResult.ok(service.create(UserContext.getUserId(), request)); }
    @PutMapping("/{id}") public ApiResult<PlaylistResponse> update(@PathVariable Long id, @Validated @RequestBody PlaylistUpdateRequest request) { return ApiResult.ok(service.update(UserContext.getUserId(), id, request)); }
    @DeleteMapping("/{id}") public ApiResult<Void> delete(@PathVariable Long id) { service.delete(UserContext.getUserId(), id); return ApiResult.ok(); }
    @PostMapping("/{id}/songs") public ApiResult<AddSongsResponse> addSongs(@PathVariable Long id, @Validated @RequestBody PlaylistSongsRequest request) { return ApiResult.ok(service.addSongs(UserContext.getUserId(), id, request.getSongIds())); }
    @DeleteMapping("/{id}/songs/{songId}") public ApiResult<Void> removeSong(@PathVariable Long id, @PathVariable Long songId) { service.removeSong(UserContext.getUserId(), id, songId); return ApiResult.ok(); }
    @PutMapping("/{id}/order") public ApiResult<Void> reorder(@PathVariable Long id, @Validated @RequestBody PlaylistSongsRequest request) { service.reorder(UserContext.getUserId(), id, request.getSongIds()); return ApiResult.ok(); }
}
