package com.chengxu.futari.admin.service;

import com.chengxu.futari.admin.dto.ClientUpdateInfoResponse;
import java.nio.file.Path;
import java.util.Optional;

/** 提供兼容当前服务端版本的客户端发行信息与缓存文件。 @author Futari */
public interface ClientUpdateService {
    ClientUpdateInfoResponse updateInfo(String platform);
    Optional<Path> updateFile(String platform, String releaseTag);
    void refreshReleases();
}
