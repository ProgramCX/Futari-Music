package com.chengxu.futari.admin.controller;

import com.chengxu.futari.admin.dto.ClientUpdateInfoResponse;
import com.chengxu.futari.admin.service.ClientUpdateService;
import com.chengxu.futari.common.result.ApiResult;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import lombok.RequiredArgsConstructor;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.Resource;
import org.springframework.http.ContentDisposition;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

/** 公开更新元数据并从本机更新缓存提供安装包。 @author Futari */
@RestController
@RequestMapping("/api/updates")
@RequiredArgsConstructor
public class ClientUpdateController {
    private final ClientUpdateService service;

    @GetMapping
    public ApiResult<ClientUpdateInfoResponse> info(@RequestParam String platform) {
        return ApiResult.ok(service.updateInfo(platform));
    }

    @GetMapping("/download")
    public ResponseEntity<Resource> download(@RequestParam String platform,
                                             @RequestParam String releaseTag) {
        return service.updateFile(platform, releaseTag)
                .map(this::downloadResponse)
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    private ResponseEntity<Resource> downloadResponse(Path file) {
        String fileName = file.getFileName().toString();
        return ResponseEntity.ok()
                .contentType(MediaType.APPLICATION_OCTET_STREAM)
                .contentLength(file.toFile().length())
                .header(HttpHeaders.CACHE_CONTROL, "no-store")
                .header(HttpHeaders.CONTENT_DISPOSITION,
                        ContentDisposition.attachment().filename(fileName, StandardCharsets.UTF_8).build().toString())
                .body(new FileSystemResource(file));
    }
}
