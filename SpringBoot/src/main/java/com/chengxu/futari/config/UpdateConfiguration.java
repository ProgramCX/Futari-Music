package com.chengxu.futari.config;

import org.springframework.boot.context.properties.EnableConfigurationProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.concurrent.ThreadPoolTaskScheduler;

/** 为 GitHub 发行轮询提供独立的后台调度线程。 @author Futari */
@Configuration
@EnableScheduling
@EnableConfigurationProperties(ClientUpdateProperties.class)
public class UpdateConfiguration {
    @Bean("taskScheduler")
    public ThreadPoolTaskScheduler taskScheduler() {
        return createScheduler("futari-scheduler-");
    }

    @Bean("futariUpdateTaskScheduler")
    public ThreadPoolTaskScheduler futariUpdateTaskScheduler() {
        return createScheduler("futari-update-poller-");
    }

    private ThreadPoolTaskScheduler createScheduler(String threadNamePrefix) {
        ThreadPoolTaskScheduler scheduler = new ThreadPoolTaskScheduler();
        scheduler.setPoolSize(1);
        scheduler.setThreadNamePrefix(threadNamePrefix);
        scheduler.setWaitForTasksToCompleteOnShutdown(true);
        scheduler.setAwaitTerminationSeconds(15);
        return scheduler;
    }
}
