package com.chengxu.futari.song.controller;

import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.song.dto.*;
import com.chengxu.futari.song.service.CoverCompletionService;
import com.chengxu.futari.common.result.PageResult;
import lombok.RequiredArgsConstructor;
import org.springframework.http.MediaType;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 封面补全专用管理接口，歌曲与专辑独立确认。 @author Futari */
@RestController @RequestMapping("/api/cover-completion") @RequiredArgsConstructor
public class CoverCompletionController {
    private final CoverCompletionService service;

    @RequirePermission("admin") @GetMapping("/songs")
    public ApiResult<PageResult<CoverCandidateResponse>> songs(@RequestParam(defaultValue = "1") int pageNum,
            @RequestParam(defaultValue = "100") int pageSize) {
        return ApiResult.ok(service.songs(pageNum, pageSize));
    }

    @RequirePermission("admin") @GetMapping("/albums")
    public ApiResult<PageResult<AlbumResponse>> albums(@RequestParam(defaultValue = "1") int pageNum,
            @RequestParam(defaultValue = "100") int pageSize) {
        return ApiResult.ok(service.albums(pageNum, pageSize));
    }

    @RequirePermission("admin") @PostMapping(value = "/songs/{id}", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ApiResult<CoverCompletionResponse> song(@PathVariable Long id,
            @Validated @ModelAttribute CoverCompletionRequest request) {
        return ApiResult.ok(service.completeSong(id, request));
    }

    @RequirePermission("admin") @PostMapping(value = "/albums/{id}", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ApiResult<CoverCompletionResponse> album(@PathVariable Long id,
            @Validated @ModelAttribute CoverCompletionRequest request) {
        return ApiResult.ok(service.completeAlbum(id, request));
    }
}
