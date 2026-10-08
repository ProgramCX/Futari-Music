package com.chengxu.futari.song.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 去重后的歌曲与文件元数据。 @author Futari */
@Getter @Setter @TableName("song")
public class Song {
    @TableId(type = IdType.AUTO) private Long id;
    private String hash;
    private String title;
    private String artist;
    private Long albumId;
    private String album;
    @TableField(select = false) private String lyrics;
    private Boolean hasLyrics;
    private String coverHash;
    private String coverFormat;
    private Integer durationMs;
    private Long fileSize;
    private String format;
    private Long uploaderId;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
