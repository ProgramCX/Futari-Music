package com.chengxu.futari.room.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 房间持久化元数据。 @author Futari */
@Getter @Setter @TableName("room")
public class Room {
    @TableId(type = IdType.AUTO) private Long id;
    private String name;
    private Long ownerId;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
