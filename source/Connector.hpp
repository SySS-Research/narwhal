#pragma once

#include "common.hpp"

#include <string>
#include <memory>
#include <functional>
#include <list>
#include <span>

namespace emu
{

class Connector
{
public:
    Connector(const std::string& id, const std::string& type)
     : mCallback(), mId(id), mTypeId(type)
    {
    }

    using GenericCallbackFn = std::function<uint64_t(uint64_t, uint64_t, std::span<const uint8_t>)>;

    uint64_t operator()(uint64_t arg0, uint64_t arg1, std::span<const uint8_t> dataArg)
    {
        if (!mCallback) {
            EMU_LOG_WARN("Connector missing callback");
            return 0;
        }

        return mCallback(arg0, arg1, std::move(dataArg));
    }

    void SetCallback(GenericCallbackFn callback)
    {
        mCallback = std::move(callback);
    }

    GenericCallbackFn GetCallback() const
    {
        return mCallback;
    }

    void Connect(std::shared_ptr<Connector> other)
    {
        SetCallback(other->GetCallback());
    }

    const std::string& GetId() const
    {
        return mId;
    }

    const std::string& GetType() const
    {
        return mTypeId;
    }

protected:
    GenericCallbackFn mCallback;
    std::string mId;
    std::string mTypeId;
};

template<typename Impl>
class ConnectorView
{
public:
    explicit ConnectorView(const std::string& id)
     : mConnector(std::make_shared<emu::Connector>(id, std::string(Impl::sTypeId))),
       mFunction()
    {
    }

    bool IsConnected() const
    {
        return !!mConnector->GetCallback();
    }

    operator bool()
    {
        return IsConnected();
    }

    std::shared_ptr<emu::Connector> GetConnector() const
    {
        return mConnector;
    }

    template<typename... Args>
    auto operator()(Args&&... args)
    {
        return Impl::Serialize(*mConnector, std::forward<Args>(args)...);
    }

    void OnCall(std::function<typename Impl::CallSignature> function)
    {
        mFunction = std::move(function);

        mConnector->SetCallback([this](uint64_t arg0, uint64_t arg1, std::span<const uint8_t> data) -> uint64_t {
            return Impl::Deserialize(arg0, arg1, data, mFunction);
        });
    }

private:
    std::shared_ptr<emu::Connector> mConnector;
    std::function<typename Impl::CallSignature> mFunction;
};

class ConnectorStore
{
public:
    ConnectorStore() = default;
    ~ConnectorStore() = default;

    std::shared_ptr<Connector> Get(const std::string& id)
    {
        for (auto it = mConnectors.begin(); it != mConnectors.end(); it++) {
            if ((*it)->GetId() == id) {
                return *it;
            }
        }

        return nullptr;
    }

    void Add(std::shared_ptr<Connector> connector)
    {
        mConnectors.push_back(connector);
    }

    template<typename T>
    void Add(const ConnectorView<T>& view)
    {
        Add(view.GetConnector());
    }

    auto begin() { return mConnectors.begin(); }
    auto end() { return mConnectors.end(); }

private:
    std::list<std::shared_ptr<Connector>> mConnectors;
};

class IConnectable
{
public:
    IConnectable();
    virtual ~IConnectable();

    ConnectorStore& GetInputConnectors() { return mInputConnectors; }
    ConnectorStore& GetOutputConnectors() { return mOutputConnectors; }

    std::optional<std::string> GetIdentifier() const { return mIdentifier; }
    void SetIdentifier(std::string identifier) { mIdentifier = std::move(identifier); }

    const std::string& GetName() const { return mName; }
    void SetName(std::string name) { mName = std::move(name); }

private:
    ConnectorStore mInputConnectors;
    ConnectorStore mOutputConnectors;
    
    std::optional<std::string> mIdentifier;
    std::string mName;
};

} // namespace emu
