package com.chengxu.futari.playlist.personal.dto;

import com.chengxu.futari.song.dto.SongResponse;
import java.util.List;
import lombok.AllArgsConstructor;
import lombok.Getter;

/** 个人歌单及其歌曲。 @author Futari */
@Getter @AllArgsConstructor
public class PlaylistResponse {
    private Long id;
    private String name;
    private String description;
    private String coverUrl;
    private Long creatorId;
    private Long createdAt;
    private List<SongResponse> songs;
}
