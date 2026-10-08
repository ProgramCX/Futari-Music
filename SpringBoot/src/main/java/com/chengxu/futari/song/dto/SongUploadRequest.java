package com.chengxu.futari.song.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.Setter;
import org.springframework.web.multipart.MultipartFile;

/** 音频、歌词和专辑图片的 multipart 上传参数。 @author Futari */
@Getter @Setter
public class SongUploadRequest {
    @NotNull private MultipartFile file;
    @NotBlank @Size(max = 128) private String title;
    @Size(max = 128) private String artist;
    @Size(max = 128) private String album;
    private Long albumId;
    @Size(max = 128) private String newAlbumName;
    @Size(max = 128) private String newAlbumArtist;
    private Integer durationMs;
    @Size(max = 262144) private String lyricsText;
    private MultipartFile lyricsFile;
    private MultipartFile coverFile;
    private MultipartFile albumCoverFile;
    private MultipartFile songCoverFile;
}
