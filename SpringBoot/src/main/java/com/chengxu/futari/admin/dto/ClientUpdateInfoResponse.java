package com.chengxu.futari.admin.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 客户端可公开读取的服务端版本及兼容客户端发行信息。 @author Futari */
@Getter
@AllArgsConstructor
public class ClientUpdateInfoResponse {
    private String serverVersion;
    private String serverCompatibilityLine;
    private String clientVersion;
    private String releaseTag;
    private String fileName;
    private long fileSize;
    private String sha256;
    private String downloadUrl;
}
