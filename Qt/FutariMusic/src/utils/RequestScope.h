#ifndef FUTARI_REQUEST_SCOPE_H
#define FUTARI_REQUEST_SCOPE_H

#include <memory>

// 仅在所属 QObject 的线程使用。回调只持有弱引用，不延长请求域的生命周期。
// renew 使此前所有 token 失效；销毁/替换请求域同样使旧回调失效。
class RequestScope {
public:
    using Token = std::weak_ptr<const int>;
    RequestScope() = default;
    RequestScope(const RequestScope&) = delete;
    RequestScope& operator=(const RequestScope&) = delete;
    RequestScope(RequestScope&&) = default;
    RequestScope& operator=(RequestScope&&) = default;

    Token token() const { return m_identity; }
    Token renew() {
        m_identity = std::make_shared<const int>(0);
        return token();
    }

private:
    std::shared_ptr<const int> m_identity = std::make_shared<const int>(0);
};

#endif  // FUTARI_REQUEST_SCOPE_H
