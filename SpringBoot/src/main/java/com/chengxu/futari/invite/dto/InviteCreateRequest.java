package com.chengxu.futari.invite.dto;

import jakarta.validation.constraints.NotNull;
import lombok.Getter;
import lombok.Setter;

/** 房间邀请目标。 @author Futari */
@Getter @Setter
public class InviteCreateRequest { @NotNull private Long partnerId; }
