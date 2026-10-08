package com.chengxu.futari.common.context;

import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;

/** 当前请求的用户身份。 @author Futari */
public final class UserContext {
    private static final ThreadLocal<Long> USER_ID = new ThreadLocal<>();
    private UserContext() { }
    public static void setUserId(Long userId) { USER_ID.set(userId); }
    public static Long getUserId() {
        Long id = USER_ID.get();
        if (id == null) throw new BizException(ErrorCode.UNAUTHORIZED);
        return id;
    }
    public static void clear() { USER_ID.remove(); }
}
