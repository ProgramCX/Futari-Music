package com.chengxu.futari.auth.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.Setter;

/** 注册请求。 @author Futari */
@Getter @Setter
public class RegisterRequest {
    @NotBlank @Size(min = 3, max = 32) private String username;
    @NotBlank @Size(min = 1, max = 32) private String nickname;
    @NotBlank @Size(min = 8, max = 72) private String password;
}
