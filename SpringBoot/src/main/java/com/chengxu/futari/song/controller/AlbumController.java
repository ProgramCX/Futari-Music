package com.chengxu.futari.song.controller;

import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.AlbumCoverResponse;
import com.chengxu.futari.song.dto.AlbumResponse;
import com.chengxu.futari.song.service.AlbumService;
import jakarta.servlet.http.HttpServletRequest;
import lombok.RequiredArgsConstructor;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

/** 专辑列表和专辑封面。 @author Futari */
@RestController @RequestMapping("/api/albums") @RequiredArgsConstructor
public class AlbumController {
    private final AlbumService service;

    @GetMapping public ApiResult<PageResult<AlbumResponse>> list(
            @RequestParam(defaultValue = "") String keyword,
            @RequestParam(defaultValue = "1") int pageNum,
            @RequestParam(defaultValue = "100") int pageSize) {
        return ApiResult.ok(service.list(keyword, pageNum, pageSize));
    }

    @GetMapping("/{id}/cover") public ResponseEntity<Resource> cover(@PathVariable Long id, HttpServletRequest request) {
        AlbumCoverResponse cover = service.cover(id);
        if ("1".equals(request.getHeader("X-Futari-Nginx"))) {
            return ResponseEntity.ok().header("X-Accel-Redirect", cover.getInternalPath())
                    .header(HttpHeaders.CACHE_CONTROL, "private, no-store").build();
        }
        return ResponseEntity.ok().contentType(MediaType.parseMediaType(cover.getContentType()))
                .contentLength(cover.getFileSize()).header(HttpHeaders.CACHE_CONTROL, "private, no-store")
                .body(new FileSystemResource(cover.getFilePath()));
    }
}
