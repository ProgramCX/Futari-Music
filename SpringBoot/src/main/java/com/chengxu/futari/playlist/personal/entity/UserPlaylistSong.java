package com.chengxu.futari.playlist.personal.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 个人歌单歌曲关系。 @author Futari */
@Getter @Setter @TableName("user_playlist_song")
public class UserPlaylistSong {
    @TableId(type = IdType.AUTO) private Long id;
    private Long playlistId;
    private Long songId;
    private Integer sortOrder;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
