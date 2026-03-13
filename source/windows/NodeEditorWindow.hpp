#pragma once

#include <list>
#include <vector>
#include <optional>
#include <memory>

#include "Window.hpp"

namespace emu { class Device; }

class Attribute
{
public:
    enum Type {
        Input,
        Output,
    };

    Attribute(Type type, std::string name);
    ~Attribute();

    void Draw();

    int GetID() const { return mID; }
    int GetType() const { return mType; }
    const std::string& GetName() const { return mName; }

private:
    int mID;
    Type mType;
    std::string mName;
};

class Node
{
public:
    enum Type {
        CPU,
        Peripheral,
        Device,
    };

public:
    Node(Type type, std::string name);
    ~Node();

    void Draw();

    Attribute& AddInputAttribute(std::string name);
    Attribute& AddOutputAttribute(std::string name);

    int GetID() const { return mID; }
    const std::string& GetName() const { return mName; }

    void SetPosition(float x, float y);
    void SetDevice(std::shared_ptr<emu::Device> device);

    std::optional<Attribute> GetInputAttribute(const std::string& name);
    std::optional<Attribute> GetOutputAttribute(const std::string& name);

    auto begin() { return mAttributes.begin(); }
    auto end() { return mAttributes.end(); }

private:
    int mID;
    Type mType;
    std::string mName;
    std::list<Attribute> mAttributes;

    std::shared_ptr<emu::Device> mDevice;
};

class NodeEditorWindow : public emu::Window
{
public:
    NodeEditorWindow();
    virtual ~NodeEditorWindow();

    virtual void Init() override;
    virtual void Draw() override;

    std::optional<Node> GetNode(const std::string& identifier);

private:
    std::list<Node> mNodes;
    std::vector<std::tuple<int, int, int>> mLinks;
};
