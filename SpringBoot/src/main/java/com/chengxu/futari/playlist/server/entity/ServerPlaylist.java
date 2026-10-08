package com.chengxu.futari.playlist.server.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 公共服务器歌单。 @author Futari */
@Getter @Setter @TableName("server_playlist")
public class ServerPlaylist {
    @TableId(type = IdType.AUTO) private Long id;
    private String name;
    private String description;
    private String coverUrl;
    private Long creatorId;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
