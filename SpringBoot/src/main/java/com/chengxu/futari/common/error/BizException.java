package com.chengxu.futari.common.error;

import lombok.Getter;

/** 可向客户端展示的业务异常。 @author Futari */
@Getter
public class BizException extends RuntimeException {
    private final ErrorCode errorCode;
    public BizException(ErrorCode errorCode) { super(errorCode.getMessage()); this.errorCode = errorCode; }
}
