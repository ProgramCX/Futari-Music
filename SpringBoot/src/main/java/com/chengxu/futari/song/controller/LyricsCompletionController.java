package com.chengxu.futari.song.controller;

import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.LyricsCandidateResponse;
import com.chengxu.futari.song.dto.LyricsCompletionRequest;
import com.chengxu.futari.song.dto.LyricsCompletionResponse;
import com.chengxu.futari.song.service.LyricsCompletionService;
import lombok.RequiredArgsConstructor;
import org.springframework.http.MediaType;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.ModelAttribute;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

/** 歌词补全管理接口，按类型查询并补入空歌词字段。 @author Futari */
@RestController
@RequestMapping("/api/lyrics-completion")
@RequiredArgsConstructor
public class LyricsCompletionController {
    private final LyricsCompletionService service;

    @RequirePermission("admin")
    @GetMapping("/{type}")
    public ApiResult<PageResult<LyricsCandidateResponse>> candidates(
            @PathVariable String type,
            @RequestParam(defaultValue = "1") int pageNum,
            @RequestParam(defaultValue = "100") int pageSize) {
        return ApiResult.ok(service.candidates(type, pageNum, pageSize));
    }

    @RequirePermission("admin")
    @PostMapping(value = "/{type}/{id}",
            consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ApiResult<LyricsCompletionResponse> complete(
            @PathVariable String type,
            @PathVariable Long id,
            @Validated @ModelAttribute LyricsCompletionRequest request) {
        return ApiResult.ok(service.complete(id, type, request));
    }
}
