package com.chengxu.futari.song.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 专辑公开信息。 @author Futari */
@Getter @AllArgsConstructor
public class AlbumResponse {
    private Long id;
    private String name;
    private String artist;
    private String coverUrl;
}
