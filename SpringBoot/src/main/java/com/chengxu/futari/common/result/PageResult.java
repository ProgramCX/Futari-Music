package com.chengxu.futari.common.result;

import java.util.List;
import lombok.AllArgsConstructor;
import lombok.Getter;

/** 分页结果。 @author Futari */
@Getter @AllArgsConstructor
public class PageResult<T> {
    private long total;
    private List<T> list;
}
