#include "NodeEditorWindow.hpp"
#include "Application.hpp"
#include "Device.hpp"

#include <imgui.h>
#include <imnodes.h>

namespace
{
    
int gIDGen = 1;

} // namespace

Attribute::Attribute(Type type, std::string name)
 : mID(gIDGen++),
 mType(type),
 mName(name)
{
}

Attribute::~Attribute()
{
}

void Attribute::Draw()
{
    mType == Input ? ImNodes::BeginInputAttribute(mID) : ImNodes::BeginOutputAttribute(mID);
    {
        ImGui::Text(mName.c_str());

    }
    mType == Input ? ImNodes::EndInputAttribute() : ImNodes::EndOutputAttribute();
}

Node::Node(Type type, std::string name)
 : mID(gIDGen++),
 mType(type),
 mName(std::move(name)),
 mAttributes(),
 mDevice()
{
}

Node::~Node()
{
}

void Node::Draw()
{
    uint32_t color = 0;
    switch (mType) {
        case Type::CPU:
            color = IM_COL32(0x2F, 0x5D, 0x8A, 0xFF);
            break;
        case Type::Peripheral:
            color = IM_COL32(0xC8, 0x89, 0x1A, 0xFF);
            break;
        case Type::Device:
            color = IM_COL32(0x1E, 0x7F, 0x7A, 0xFF);
            break;
    }
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, color);

    ImNodes::BeginNode(mID);
    {
        ImNodes::BeginNodeTitleBar();
        {
            ImGui::TextUnformatted(mName.c_str());
        }
        ImNodes::EndNodeTitleBar();

        if (mDevice) {
            mDevice->DrawNode();
        }

        // This is necessary apparently
        if (mAttributes.empty()) {
            ImGui::Dummy(ImVec2(80.0f, 45.0f));
        }

        // Draw attributes
        for (auto& attr : mAttributes) {
            attr.Draw();
        }
    }
    ImNodes::EndNode();

    ImNodes::PopColorStyle();
}

Attribute& Node::AddInputAttribute(std::string name)
{
    return mAttributes.emplace_back(Attribute::Type::Input, name);
}

Attribute& Node::AddOutputAttribute(std::string name)
{
    return mAttributes.emplace_back(Attribute::Type::Output, name);
}

void Node::SetPosition(float x, float y)
{
    ImNodes::SetNodeEditorSpacePos(mID, ImVec2(x, y));
}

void Node::SetDevice(std::shared_ptr<emu::Device> device)
{
    mDevice = std::move(device);
}

std::optional<Attribute> Node::GetInputAttribute(const std::string& name)
{
    for (auto& attribute : mAttributes) {
        if (attribute.GetType() == Attribute::Type::Input && attribute.GetName() == name) {
            return attribute;
        }
    }

    return std::nullopt;
}

std::optional<Attribute> Node::GetOutputAttribute(const std::string& name)
{
    for (auto& attribute : mAttributes) {
        if (attribute.GetType() == Attribute::Type::Output && attribute.GetName() == name) {
            return attribute;
        }
    }

    return std::nullopt;
}

NodeEditorWindow::NodeEditorWindow()
 : Window("Node Editor")
{
}

NodeEditorWindow::~NodeEditorWindow()
{
}

void NodeEditorWindow::Init()
{
    auto& cpu = mNodes.emplace_back(Node::Type::CPU, "CPU");
    cpu.SetPosition(0.0f, 0.0f);
    auto& cpuPin = cpu.AddOutputAttribute("");

    auto config = emu::Application::Get()->GetConfiguration();

    float curYPos = 0.0f;
    for (auto peripheral : config->GetPeripherals()) {
        auto& node = mNodes.emplace_back(Node::Type::Peripheral, peripheral->GetName());

        node.SetPosition(200.0f, curYPos);
        curYPos += 100.0f;

        auto& attr = node.AddInputAttribute("");
        mLinks.emplace_back(gIDGen++, cpuPin.GetID(), attr.GetID());

        for (auto& connector : peripheral->GetInputConnectors()) {
            node.AddInputAttribute(connector->GetId());
        }

        for (auto& connector : peripheral->GetOutputConnectors()) {
            node.AddOutputAttribute(connector->GetId());
        }
    }

    // Set CPU in the middle
    cpu.SetPosition(0.0f, curYPos / 2.0f);

    curYPos = 0.0f;
    for (auto device : config->GetDevices()) {
        auto& node = mNodes.emplace_back(Node::Type::Device, device->GetName());
        node.SetDevice(device);

        node.SetPosition(500.0f, curYPos);
        curYPos += 100.0f;

        for (auto& connector : device->GetInputConnectors()) {
            node.AddInputAttribute(connector->GetId());
        }

        for (auto& connector : device->GetOutputConnectors()) {
            node.AddOutputAttribute(connector->GetId());
        }
    }

    // TODO this is not great
    auto& connection = config->GetConnections();
    for (auto& [fromIdentifier, fromPort, toIdentifier, toPort] : connection) {
        auto fromNode = GetNode(fromIdentifier);
        auto toNode = GetNode(toIdentifier);
        EMU_ASSERT(fromNode && toNode);

        auto fromAttribute = fromNode->GetOutputAttribute(fromPort);
        auto toAttribute = toNode->GetInputAttribute(toPort);
        EMU_ASSERT(fromAttribute && toAttribute);

        mLinks.emplace_back(gIDGen++, fromAttribute->GetID(), toAttribute->GetID());
    }
}

void NodeEditorWindow::Draw()
{
    ImNodes::BeginNodeEditor();
    {
        for (auto& node : mNodes) {
            node.Draw();
        }

        for (auto& [id, a, b] : mLinks) {
            ImNodes::Link(id, a, b);
        }

        ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
    }
    ImNodes::EndNodeEditor();
}

std::optional<Node> NodeEditorWindow::GetNode(const std::string& identifier)
{
    for (auto& node : mNodes) {
        // TODO don't check name, might not always be identifier
        if (node.GetName() == identifier) {
            return node;
        }
    }

    return std::nullopt;
}
