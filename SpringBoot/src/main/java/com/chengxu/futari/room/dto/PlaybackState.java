package com.chengxu.futari.room.dto;

import lombok.Getter;
import lombok.Setter;

/** 可按服务器时间推算位置的播放状态。 @author Futari */
@Getter @Setter
public class PlaybackState {
    private PlaybackMode mode = PlaybackMode.SEQUENTIAL;
    // 模式变更不移动播放时钟；独立版本号用于拒绝晚到的 REST / SYNC 快照。
    private Long revision = 0L;
    private Long currentSongId;
    private String status = "paused";
    private Long positionMs = 0L;
    private Long serverTimestamp = 0L;
}
