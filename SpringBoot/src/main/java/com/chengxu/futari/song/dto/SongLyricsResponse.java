package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 单曲歌词文本，可包含 LRC 时间标签。 @author Futari */
@Getter @AllArgsConstructor
public class SongLyricsResponse {
    private Long songId;
    private String lyrics;
}
