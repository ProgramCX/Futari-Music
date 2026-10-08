package com.chengxu.futari.playlist.server.controller;

import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.playlist.personal.dto.AddSongsResponse;
import com.chengxu.futari.playlist.personal.dto.PlaylistSongsRequest;
import com.chengxu.futari.playlist.server.dto.*;
import com.chengxu.futari.playlist.server.service.ServerPlaylistService;
import java.util.List;
import lombok.RequiredArgsConstructor;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 公共歌单接口。 @author Futari */
@RestController @RequestMapping("/api/server-playlists") @RequiredArgsConstructor
public class ServerPlaylistController {
    private final ServerPlaylistService service;
    @GetMapping public ApiResult<List<ServerPlaylistResponse>> list() { return ApiResult.ok(service.list()); }
    @GetMapping("/{id}") public ApiResult<ServerPlaylistResponse> get(@PathVariable Long id) { return ApiResult.ok(service.get(id)); }
    @RequirePermission("serverPlaylist") @PostMapping public ApiResult<ServerPlaylistResponse> create(@Validated @RequestBody ServerPlaylistCreateRequest request) { return ApiResult.ok(service.create(UserContext.getUserId(), request)); }
    @RequirePermission("serverPlaylist") @PutMapping("/{id}") public ApiResult<ServerPlaylistResponse> update(@PathVariable Long id, @Validated @RequestBody ServerPlaylistUpdateRequest request) { return ApiResult.ok(service.update(id, request)); }
    @RequirePermission("serverPlaylist") @DeleteMapping("/{id}") public ApiResult<Void> delete(@PathVariable Long id) { service.delete(id); return ApiResult.ok(); }
    @RequirePermission("serverPlaylist") @PostMapping("/{id}/songs") public ApiResult<AddSongsResponse> addSongs(@PathVariable Long id, @Validated @RequestBody PlaylistSongsRequest request) { return ApiResult.ok(service.addSongs(id, request.getSongIds())); }
    @RequirePermission("serverPlaylist") @DeleteMapping("/{id}/songs/{songId}") public ApiResult<Void> removeSong(@PathVariable Long id, @PathVariable Long songId) { service.removeSong(id, songId); return ApiResult.ok(); }
    @RequirePermission("serverPlaylist") @PutMapping("/{id}/order") public ApiResult<Void> reorder(@PathVariable Long id, @Validated @RequestBody PlaylistSongsRequest request) { service.reorder(id, request.getSongIds()); return ApiResult.ok(); }
}
