package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 已校验的歌曲封面文件与传输信息。 @author Futari */
@Getter @AllArgsConstructor
public class SongCoverResponse {
    private String contentType;
    private String filePath;
    private long fileSize;
}
