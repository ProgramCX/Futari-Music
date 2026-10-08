package com.chengxu.futari.config;

import com.chengxu.futari.auth.service.AuthService;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.sync.SyncWebSocketHandler;
import java.net.URI;
import java.util.Map;
import lombok.RequiredArgsConstructor;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpStatus;
import org.springframework.http.server.ServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;
import org.springframework.web.socket.server.HandshakeInterceptor;
import org.springframework.web.util.UriComponentsBuilder;

/** 原生 WebSocket 注册与握手鉴权。 @author Futari */
@Configuration @EnableWebSocket @RequiredArgsConstructor
public class WebSocketConfig implements WebSocketConfigurer {
    private final SyncWebSocketHandler handler;
    private final AuthService auth;
    @Value("${futari.cors.allowed-origins}") private String allowedOrigins;
    @Override public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
        registry.addHandler(handler, "/ws").addInterceptors(new HandshakeInterceptor() {
            @Override public boolean beforeHandshake(ServerHttpRequest request, ServerHttpResponse response, org.springframework.web.socket.WebSocketHandler wsHandler, Map<String, Object> attributes) {
                URI uri = request.getURI();
                String token = UriComponentsBuilder.fromUri(uri).build().getQueryParams().getFirst("token");
                try { attributes.put("userId", auth.authenticate(token == null ? "" : token)); attributes.put("token", token); return true; }
                catch (BizException ex) { response.setStatusCode(HttpStatus.UNAUTHORIZED); return false; }
            }
            @Override public void afterHandshake(ServerHttpRequest request, ServerHttpResponse response, org.springframework.web.socket.WebSocketHandler wsHandler, Exception exception) { }
        }).setAllowedOrigins(allowedOrigins.split(","));
    }
}
