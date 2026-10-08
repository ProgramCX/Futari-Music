package com.chengxu.futari.auth;

import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.security.Keys;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Date;
import javax.crypto.SecretKey;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

/** JWT 签发与验签。 @author Futari */
@Component
public class JwtUtil {
    private final SecretKey key;
    private final long ttlSeconds;
    public JwtUtil(@Value("${futari.jwt.secret}") String secret, @Value("${futari.jwt.ttl-seconds}") long ttlSeconds) {
        if (secret.getBytes(StandardCharsets.UTF_8).length < 32) throw new IllegalStateException("JWT_SECRET 至少需要 32 字节");
        this.key = Keys.hmacShaKeyFor(secret.getBytes(StandardCharsets.UTF_8));
        this.ttlSeconds = ttlSeconds;
    }
    public String issue(Long userId, String jti) {
        Instant now = Instant.now();
        return Jwts.builder().claim("userId", userId).id(jti).issuedAt(Date.from(now)).expiration(Date.from(now.plusSeconds(ttlSeconds))).signWith(key).compact();
    }
    public Claims parse(String token) {
        try { return Jwts.parser().verifyWith(key).build().parseSignedClaims(token).getPayload(); }
        catch (Exception ex) { throw new BizException(ErrorCode.UNAUTHORIZED); }
    }
    public long ttlSeconds() { return ttlSeconds; }
}
