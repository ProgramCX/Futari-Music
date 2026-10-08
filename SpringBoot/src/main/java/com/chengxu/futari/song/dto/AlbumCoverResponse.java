package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 已校验的专辑封面路径与传输信息。 @author Futari */
@Getter @AllArgsConstructor
public class AlbumCoverResponse {
    private String internalPath;
    private String contentType;
    private String filePath;
    private long fileSize;
}
