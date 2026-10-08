package com.chengxu.futari.playlist.server.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 公共歌单歌曲关系。 @author Futari */
@Getter @Setter @TableName("server_playlist_song")
public class ServerPlaylistSong {
    @TableId(type = IdType.AUTO) private Long id;
    private Long playlistId;
    private Long songId;
    private Integer sortOrder;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
