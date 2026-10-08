package com.chengxu.futari.admin.dto;

import jakarta.validation.constraints.NotNull;
import lombok.Getter;
import lombok.Setter;

/** 管理端权限修改请求。 @author Futari */
@Getter @Setter
public class PermissionsUpdateRequest {
    @NotNull private Boolean canUpload;
    @NotNull private Boolean canManageServerPlaylist;
}
