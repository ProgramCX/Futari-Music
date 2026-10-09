package com.chengxu.futari.config;

import org.junit.jupiter.api.Test;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;

import static org.junit.jupiter.api.Assertions.assertNotSame;

/** 防止更新轮询占用服务端现有的定时同步线程。 @author Futari */
class UpdateConfigurationTest {
    @Test
    void updatePollerHasAnIndependentScheduler() {
        try (var context = new AnnotationConfigApplicationContext(UpdateConfiguration.class)) {
            assertNotSame(context.getBean("taskScheduler"),
                    context.getBean("futariUpdateTaskScheduler"));
        }
    }
}
