package com.chengxu.futari.config;

import lombok.RequiredArgsConstructor;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.config.annotation.CorsRegistry;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

/** REST 拦截器与跨域配置。 @author Futari */
@Configuration @RequiredArgsConstructor
public class WebConfig implements WebMvcConfigurer {
    private final AuthInterceptor auth;
    @Value("${futari.cors.allowed-origins}") private String allowedOrigins;
    @Override public void addInterceptors(InterceptorRegistry registry) {
        registry.addInterceptor(auth).addPathPatterns("/api/**")
                .excludePathPatterns("/api/auth/register", "/api/auth/login", "/api/updates/**");
    }
    @Override public void addCorsMappings(CorsRegistry registry) {
        registry.addMapping("/api/**").allowedOrigins(allowedOrigins.split(",")).allowedMethods("GET", "POST", "PUT", "DELETE", "OPTIONS").allowedHeaders("Authorization", "Content-Type");
    }
}
