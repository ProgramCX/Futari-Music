package com.chengxu.futari.playlist.personal.dto;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** 幂等批量添加计数。 @author Futari */
@Getter @AllArgsConstructor
public class AddSongsResponse { private int added; private int duplicated; }
