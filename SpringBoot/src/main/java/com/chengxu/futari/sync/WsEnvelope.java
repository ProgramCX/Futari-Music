package com.chengxu.futari.sync;

import lombok.AllArgsConstructor;
import lombok.Getter;

/** WebSocket 下行统一信封。 @author Futari */
@Getter @AllArgsConstructor
public class WsEnvelope {
    private String type;
    private Long serverTime;
    private Object data;
}
