package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 歌曲公开元数据。 @author Futari */
@Getter @AllArgsConstructor
public class SongResponse {
    private Long id;
    private String hash;
    private String title;
    private String artist;
    private Long albumId;
    private String album;
    private String albumArtist;
    private Boolean hasLyrics;
    private String coverUrl;
    private Integer durationMs;
    private Long fileSize;
    private String format;
    private Long uploaderId;
    private Long createdAt;
}
