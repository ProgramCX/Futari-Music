package com.chengxu.futari.playlist.server.dto;

import com.chengxu.futari.song.dto.SongResponse;
import java.util.List;
import lombok.AllArgsConstructor;
import lombok.Getter;

/** 公共歌单及歌曲。 @author Futari */
@Getter @AllArgsConstructor
public class ServerPlaylistResponse {
    private Long id;
    private String name;
    private String description;
    private String coverUrl;
    private Long creatorId;
    private Long createdAt;
    private List<SongResponse> songs;
}
