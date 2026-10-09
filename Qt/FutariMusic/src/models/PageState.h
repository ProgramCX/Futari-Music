#ifndef FUTARI_PAGE_STATE_H
#define FUTARI_PAGE_STATE_H
#include <QJsonArray>
#include <QString>

#include "../utils/RequestScope.h"

// 一个搜索结果集的生命周期。刷新时整体替换，使旧请求的 token 失效。
struct PageState {
    QJsonArray items;
    QString keyword;
    int page = 0;          // 最后成功加载的页；失败不推进，以便重试相同页。
    int total = 0;         // 刷新时设为 1 允许首请求，随后采用服务端总数。
    bool loading = false;  // 同一结果集至多一页在途；成功/失败均释放。
    RequestScope requests;
};
#endif  // FUTARI_PAGE_STATE_H
