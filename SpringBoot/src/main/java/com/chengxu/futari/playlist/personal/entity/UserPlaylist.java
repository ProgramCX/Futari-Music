package com.chengxu.futari.playlist.personal.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 用户个人歌单。 @author Futari */
@Getter @Setter @TableName("user_playlist")
public class UserPlaylist {
    @TableId(type = IdType.AUTO) private Long id;
    private Long userId;
    private String name;
    private String description;
    private String coverUrl;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
