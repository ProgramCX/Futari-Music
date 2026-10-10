package com.chengxu.futari.song.dto;

import jakarta.validation.constraints.NotNull;
import lombok.Getter;
import lombok.Setter;
import org.springframework.web.multipart.MultipartFile;

/** 单个确认后的封面文件；目标由路由指定。 @author Futari */
@Getter @Setter
public class CoverCompletionRequest {
    @NotNull private MultipartFile file;
}
