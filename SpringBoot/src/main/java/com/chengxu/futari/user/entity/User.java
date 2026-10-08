package com.chengxu.futari.user.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 用户持久化实体。 @author Futari */
@Getter @Setter @TableName("user")
public class User {
    @TableId(type = IdType.AUTO) private Long id;
    private String username;
    private String nickname;
    private String avatarUrl;
    private String passwordHash;
    private String role;
    private Boolean canUpload;
    private Boolean canManageServerPlaylist;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
