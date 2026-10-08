package com.chengxu.futari.config;

import com.chengxu.futari.auth.service.AuthService;
import com.chengxu.futari.common.annotation.RequirePermission;
import com.chengxu.futari.common.context.UserContext;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.user.service.UserService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Component;
import org.springframework.web.method.HandlerMethod;
import org.springframework.web.servlet.HandlerInterceptor;

/** REST 令牌与声明式权限校验。 @author Futari */
@Component @RequiredArgsConstructor
public class AuthInterceptor implements HandlerInterceptor {
    private final AuthService auth;
    private final UserService users;
    @Override public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) {
        if ("OPTIONS".equals(request.getMethod())) return true;
        String header = request.getHeader("Authorization");
        if (header == null || !header.startsWith("Bearer ")) throw new BizException(ErrorCode.UNAUTHORIZED);
        Long userId = auth.authenticate(header.substring(7));
        if (handler instanceof HandlerMethod method) {
            RequirePermission requirement = method.getMethodAnnotation(RequirePermission.class);
            if (requirement == null) requirement = method.getBeanType().getAnnotation(RequirePermission.class);
            if (requirement != null && !users.hasPermission(userId, requirement.value())) throw new BizException(ErrorCode.FORBIDDEN);
        }
        UserContext.setUserId(userId);
        return true;
    }
    @Override public void afterCompletion(HttpServletRequest request, HttpServletResponse response, Object handler, Exception ex) { UserContext.clear(); }
}
