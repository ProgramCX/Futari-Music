package com.chengxu.futari.playlist.personal.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.Setter;

/** 修改个人歌单。 @author Futari */
@Getter @Setter
public class PlaylistUpdateRequest {
    @NotBlank @Size(max = 64) private String name;
    @Size(max = 255) private String description;
    @Size(max = 255) private String coverUrl;
}
