package com.chengxu.futari.partner.entity;

import com.baomidou.mybatisplus.annotation.*;
import java.time.LocalDateTime;
import lombok.Getter;
import lombok.Setter;

/** 用户单向收藏关系。 @author Futari */
@Getter @Setter @TableName("favorite_partner")
public class FavoritePartner {
    @TableId(type = IdType.AUTO) private Long id;
    private Long userId;
    private Long partnerId;
    private String remark;
    @TableField(fill = FieldFill.INSERT) private LocalDateTime createdAt;
    @TableField(fill = FieldFill.INSERT_UPDATE) private LocalDateTime updatedAt;
    @TableLogic private Integer deleted;
}
