package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 已校验的封面内部路径与类型。 @author Futari */
@Getter @AllArgsConstructor
public class SongCoverResponse {
    private String internalPath;
    private String contentType;
    private String filePath;
    private long fileSize;
}
