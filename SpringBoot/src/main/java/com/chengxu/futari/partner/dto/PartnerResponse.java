package com.chengxu.futari.partner.dto;

import com.chengxu.futari.user.dto.UserResponse;
import lombok.AllArgsConstructor;
import lombok.Getter;

/** 搭子及其在线状态。 @author Futari */
@Getter @AllArgsConstructor
public class PartnerResponse {
    private UserResponse user;
    private String remark;
}
