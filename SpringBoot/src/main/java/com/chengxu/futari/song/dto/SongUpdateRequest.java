package com.chengxu.futari.song.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.Setter;
import org.springframework.web.multipart.MultipartFile;

/** 管理员修改歌曲和共享专辑信息的 multipart 参数。 @author Futari */
@Getter @Setter
public class SongUpdateRequest {
    private MultipartFile file;
    @NotBlank @Size(max = 128) private String title;
    @Size(max = 128) private String artist;
    private Long albumId;
    @Size(max = 128) private String albumName;
    @Size(max = 128) private String albumArtist;
    @Size(max = 128) private String newAlbumName;
    @Size(max = 128) private String newAlbumArtist;
    private Integer durationMs;
    @Size(max = 262144) private String lyricsText;
    private MultipartFile lyricsFile;
    private MultipartFile albumCoverFile;
    private MultipartFile songCoverFile;
}
