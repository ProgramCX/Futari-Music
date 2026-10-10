package com.chengxu.futari.song.dto;

/** false 表示封面已被其他操作补全，本次未覆盖。 @author Futari */
public record CoverCompletionResponse(boolean applied) { }
