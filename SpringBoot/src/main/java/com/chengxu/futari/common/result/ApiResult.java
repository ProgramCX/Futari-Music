package com.chengxu.futari.common.result;

import com.chengxu.futari.common.error.ErrorCode;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;

/** REST 统一响应。 @author Futari */
@Getter @Setter @NoArgsConstructor @AllArgsConstructor
public class ApiResult<T> {
    private Integer code;
    private String message;
    private T data;
    public static <T> ApiResult<T> ok(T data) { return new ApiResult<>(0, "ok", data); }
    public static ApiResult<Void> ok() { return new ApiResult<>(0, "ok", null); }
    public static <T> ApiResult<T> fail(ErrorCode error) { return new ApiResult<>(error.getCode(), error.getMessage(), null); }
}
