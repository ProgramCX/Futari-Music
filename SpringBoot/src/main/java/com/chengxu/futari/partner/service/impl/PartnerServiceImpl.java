package com.chengxu.futari.partner.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.chengxu.futari.common.constant.WsTypes;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.partner.dto.PartnerResponse;
import com.chengxu.futari.partner.entity.FavoritePartner;
import com.chengxu.futari.partner.mapper.FavoritePartnerMapper;
import com.chengxu.futari.partner.service.PartnerService;
import com.chengxu.futari.sync.SyncPublisher;
import com.chengxu.futari.user.service.UserService;
import java.util.List;
import lombok.RequiredArgsConstructor;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.stereotype.Service;

/** 单向搭子收藏实现。 @author Futari */
@Service @RequiredArgsConstructor
public class PartnerServiceImpl implements PartnerService {
    private final FavoritePartnerMapper mapper;
    private final UserService users;
    private final SyncPublisher publisher;
    public PartnerResponse add(Long userId, Long partnerId, String remark) {
        if (userId.equals(partnerId)) throw new BizException(ErrorCode.PARTNER_SELF);
        users.require(partnerId);
        FavoritePartner existing = mapper.selectOne(new LambdaQueryWrapper<FavoritePartner>().eq(FavoritePartner::getUserId, userId).eq(FavoritePartner::getPartnerId, partnerId));
        if (existing == null) {
            FavoritePartner row = new FavoritePartner(); row.setUserId(userId); row.setPartnerId(partnerId); row.setRemark(remark);
            try { mapper.insert(row); existing = row; }
            catch (DuplicateKeyException ex) { existing = mapper.selectOne(new LambdaQueryWrapper<FavoritePartner>().eq(FavoritePartner::getUserId, userId).eq(FavoritePartner::getPartnerId, partnerId)); }
        }
        return new PartnerResponse(users.response(users.require(partnerId)), existing.getRemark());
    }
    public void remove(Long userId, Long partnerId) {
        mapper.remove(userId, partnerId);
    }
    public List<PartnerResponse> list(Long userId) {
        return mapper.selectList(new LambdaQueryWrapper<FavoritePartner>().eq(FavoritePartner::getUserId, userId).orderByDesc(FavoritePartner::getId)).stream().map(row -> new PartnerResponse(users.response(users.require(row.getPartnerId())), row.getRemark())).toList();
    }
    public void publishStatus(Long userId) {
        var response = users.response(users.require(userId));
        for (Long owner : mapper.ownersOf(userId)) publisher.send(owner, WsTypes.PARTNER_STATUS, response);
    }
}
