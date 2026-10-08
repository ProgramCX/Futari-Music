package com.chengxu.futari.config;

import com.baomidou.mybatisplus.core.handlers.MetaObjectHandler;
import com.baomidou.mybatisplus.extension.plugins.MybatisPlusInterceptor;
import com.baomidou.mybatisplus.extension.plugins.inner.PaginationInnerInterceptor;
import com.baomidou.mybatisplus.annotation.DbType;
import java.time.LocalDateTime;
import org.apache.ibatis.reflection.MetaObject;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

/** MyBatis-Plus 分页与时间填充。 @author Futari */
@Configuration
public class PersistenceConfig {
    @Bean public MybatisPlusInterceptor mybatisPlusInterceptor() {
        MybatisPlusInterceptor interceptor = new MybatisPlusInterceptor();
        interceptor.addInnerInterceptor(new PaginationInnerInterceptor(DbType.MYSQL));
        return interceptor;
    }
    @Bean public MetaObjectHandler metaObjectHandler() {
        return new MetaObjectHandler() {
            @Override public void insertFill(MetaObject meta) {
                strictInsertFill(meta, "createdAt", LocalDateTime.class, LocalDateTime.now());
                strictInsertFill(meta, "updatedAt", LocalDateTime.class, LocalDateTime.now());
            }
            @Override public void updateFill(MetaObject meta) { strictUpdateFill(meta, "updatedAt", LocalDateTime.class, LocalDateTime.now()); }
        };
    }
}
