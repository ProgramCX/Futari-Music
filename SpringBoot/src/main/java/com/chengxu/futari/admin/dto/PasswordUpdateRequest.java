package com.chengxu.futari.admin.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.Setter;

/** 管理端密码重置请求。 @author Futari */
@Getter @Setter
public class PasswordUpdateRequest { @NotBlank @Size(min = 8, max = 72) private String newPassword; }
