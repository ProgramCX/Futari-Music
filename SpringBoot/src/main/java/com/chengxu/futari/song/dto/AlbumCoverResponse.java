package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 已校验的专辑封面文件与传输信息。 @author Futari */
@Getter @AllArgsConstructor
public class AlbumCoverResponse {
    private String contentType;
    private String filePath;
    private long fileSize;
}
