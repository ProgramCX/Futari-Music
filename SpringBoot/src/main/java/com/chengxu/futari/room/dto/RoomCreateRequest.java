package com.chengxu.futari.room.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;
import lombok.Getter;
import lombok.Setter;

/** 建房请求。 @author Futari */
@Getter @Setter
public class RoomCreateRequest { @NotBlank @Size(max = 64) private String name; }
