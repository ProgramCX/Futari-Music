package com.chengxu.futari.song.controller;

import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.result.ApiResult;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.SongResponse;
import com.chengxu.futari.song.dto.SongUploadRequest;
import com.chengxu.futari.song.dto.SongLyricsResponse;
import com.chengxu.futari.song.dto.SongCoverResponse;
import com.chengxu.futari.song.dto.SongFileResponse;
import com.chengxu.futari.song.dto.SongUpdateRequest;
import com.chengxu.futari.song.service.SongService;
import jakarta.servlet.http.HttpServletRequest;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import lombok.RequiredArgsConstructor;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpRange;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.core.io.InputStreamResource;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.*;

/** 歌曲检索、上传和授权后内部重定向。 @author Futari */
@RestController @RequestMapping("/api/songs") @RequiredArgsConstructor
public class SongController {
    private final SongService service;
    @GetMapping public ApiResult<PageResult<SongResponse>> list(@RequestParam(defaultValue = "") String keyword, @RequestParam(defaultValue = "1") int pageNum, @RequestParam(defaultValue = "20") int pageSize) { return ApiResult.ok(service.list(keyword, pageNum, pageSize)); }
    @GetMapping("/mine") public ApiResult<PageResult<SongResponse>> mine(@RequestParam(defaultValue = "1") int pageNum, @RequestParam(defaultValue = "40") int pageSize) { return ApiResult.ok(service.listMine(UserContext.getUserId(), pageNum, pageSize)); }
    @GetMapping("/{id}") public ApiResult<SongResponse> get(@PathVariable Long id) { return ApiResult.ok(service.response(service.require(id))); }
    @RequirePermission("upload") @PostMapping(value = "/upload", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ApiResult<SongResponse> upload(@Validated @ModelAttribute SongUploadRequest request) { return ApiResult.ok(service.upload(UserContext.getUserId(), request)); }
    @RequirePermission("admin") @PutMapping(value = "/{id}", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ApiResult<SongResponse> update(@PathVariable Long id, @Validated @ModelAttribute SongUpdateRequest request) {
        return ApiResult.ok(service.update(id, request));
    }
    @RequirePermission("admin") @DeleteMapping("/{id}")
    public ApiResult<Void> delete(@PathVariable Long id) { service.delete(id); return ApiResult.ok(); }
    @GetMapping("/{id}/lyrics")
    public ApiResult<SongLyricsResponse> lyrics(@PathVariable Long id) { return ApiResult.ok(service.lyrics(id)); }
    @GetMapping("/{id}/cover")
    public ResponseEntity<Resource> cover(@PathVariable Long id, HttpServletRequest request) {
        SongCoverResponse cover = service.cover(id);
        if ("1".equals(request.getHeader("X-Futari-Nginx"))) {
            return ResponseEntity.ok()
                    .header("X-Accel-Redirect", cover.getInternalPath())
                    .header(HttpHeaders.CACHE_CONTROL, "private, no-store")
                    .build();
        }
        return ResponseEntity.ok()
                .contentType(MediaType.parseMediaType(cover.getContentType()))
                .contentLength(cover.getFileSize())
                .header(HttpHeaders.CACHE_CONTROL, "private, no-store")
                .body(new FileSystemResource(cover.getFilePath()));
    }
    @GetMapping("/{hash}/file")
    public ResponseEntity<?> file(@PathVariable String hash, HttpServletRequest request) {
        SongFileResponse file = service.file(hash);
        if ("1".equals(request.getHeader("X-Futari-Nginx"))) {
            return ResponseEntity.ok()
                    .header("X-Accel-Redirect", "/protected-music/" + hash)
                    .header(HttpHeaders.CACHE_CONTROL, "private, no-store")
                    .build();
        }
        String rangeHeader = request.getHeader(HttpHeaders.RANGE);
        if (rangeHeader == null || rangeHeader.isBlank()) {
            return ResponseEntity.ok().contentType(MediaType.parseMediaType(file.getContentType()))
                    .contentLength(file.getFileSize()).header(HttpHeaders.ACCEPT_RANGES, "bytes")
                    .header(HttpHeaders.CACHE_CONTROL, "private, no-store")
                    .body(new FileSystemResource(file.getFilePath()));
        }
        try {
            var ranges = HttpRange.parseRanges(rangeHeader);
            if (ranges.size() != 1 || file.getFileSize() < 1) return rangeNotSatisfiable(file.getFileSize());
            HttpRange range = ranges.get(0);
            long start = range.getRangeStart(file.getFileSize());
            long end = range.getRangeEnd(file.getFileSize());
            long length = end - start + 1;
            InputStream input = Files.newInputStream(Path.of(file.getFilePath()));
            input.skipNBytes(start);
            InputStream bounded = new FilterInputStream(input) {
                private long remaining = length;
                @Override public int read() throws IOException {
                    if (remaining <= 0) return -1;
                    int value = super.read();
                    if (value >= 0) remaining--;
                    return value;
                }
                @Override public int read(byte[] bytes, int offset, int count) throws IOException {
                    if (remaining <= 0) return -1;
                    int read = super.read(bytes, offset, (int) Math.min(count, remaining));
                    if (read > 0) remaining -= read;
                    return read;
                }
            };
            return ResponseEntity.status(HttpStatus.PARTIAL_CONTENT)
                    .contentType(MediaType.parseMediaType(file.getContentType())).contentLength(length)
                    .header(HttpHeaders.ACCEPT_RANGES, "bytes")
                    .header(HttpHeaders.CONTENT_RANGE, "bytes " + start + "-" + end + "/" + file.getFileSize())
                    .header(HttpHeaders.CACHE_CONTROL, "private, no-store")
                    .body(new InputStreamResource(bounded));
        } catch (IllegalArgumentException | IOException exception) {
            return rangeNotSatisfiable(file.getFileSize());
        }
    }

    private ResponseEntity<Void> rangeNotSatisfiable(long fileSize) {
        return ResponseEntity.status(HttpStatus.REQUESTED_RANGE_NOT_SATISFIABLE)
                .header(HttpHeaders.CONTENT_RANGE, "bytes */" + fileSize)
                .header(HttpHeaders.ACCEPT_RANGES, "bytes").build();
    }
}
