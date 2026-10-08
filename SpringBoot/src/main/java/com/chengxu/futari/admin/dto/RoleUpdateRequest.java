package com.chengxu.futari.admin.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Getter;
import lombok.Setter;

/** 管理端角色修改请求。 @author Futari */
@Getter @Setter
public class RoleUpdateRequest { @NotBlank private String role; }
