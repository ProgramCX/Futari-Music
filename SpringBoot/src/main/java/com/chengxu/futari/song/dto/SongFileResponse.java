package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 已校验的歌曲文件位置与传输信息。 @author Futari */
@Getter @AllArgsConstructor
public class SongFileResponse {
    private String filePath;
    private String contentType;
    private long fileSize;
}
