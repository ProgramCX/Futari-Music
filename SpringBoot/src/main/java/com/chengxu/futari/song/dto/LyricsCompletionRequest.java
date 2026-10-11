package com.chengxu.futari.song.dto;

import jakarta.validation.constraints.NotNull;
import lombok.Getter;
import lombok.Setter;
import org.springframework.web.multipart.MultipartFile;

/** 歌词补全请求中的本地歌词文件。 @author Futari */
@Getter
@Setter
public class LyricsCompletionRequest {
    @NotNull private MultipartFile file;
}
