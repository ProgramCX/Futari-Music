package com.chengxu.futari.partner.dto;

import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.Setter;

/** 收藏搭子时的备注。 @author Futari */
@Getter @Setter
public class PartnerCreateRequest { @Size(max = 32) private String remark; }
