package com.chengxu.futari.config;

import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;

/** 客户端更新源、落盘目录与后台轮询时序。 @author Futari */
@Getter
@Setter
@ConfigurationProperties(prefix = "futari.updates")
public class ClientUpdateProperties {
    private String githubRepository = "ProgramCX/Futari-Music";
    private String directory = "./updates";
    private boolean pollEnabled = true;
    private long pollInitialDelayMs = 0;
    private long pollIntervalMs = 600_000;
}
