package com.chengxu.futari.partner.service;

import com.chengxu.futari.partner.dto.PartnerResponse;
import java.util.List;

/** 搭子收藏与状态推送。 @author Futari */
public interface PartnerService {
    PartnerResponse add(Long userId, Long partnerId, String remark);
    void remove(Long userId, Long partnerId);
    List<PartnerResponse> list(Long userId);
    void publishStatus(Long userId);
}
