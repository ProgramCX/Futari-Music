package com.chengxu.futari.auth.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Getter;
import lombok.Setter;

/** 登录请求。 @author Futari */
@Getter @Setter
public class LoginRequest {
    @NotBlank private String username;
    @NotBlank private String password;
}
