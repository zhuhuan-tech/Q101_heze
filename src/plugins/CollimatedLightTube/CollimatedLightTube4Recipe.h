#pragma once

#include <QObject>
#include "collimatedlighttube_global.h"
#include "ObjectManager.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/behavior_tree.h"

using BT::NodeStatus;

class COLLIMATEDLIGHTTUBE_EXPORT CollimatedLightTube4Recipe : public QObject
{
	Q_OBJECT
public:
	static CollimatedLightTube4Recipe* getInstance();
	explicit CollimatedLightTube4Recipe(QObject* parent = nullptr);
	~CollimatedLightTube4Recipe();

	QString getNodeValueByName(BT::TreeNode& node, std::string name);

	NodeStatus CLT_Connect();
	NodeStatus CLT_Close();
	NodeStatus CLT_SetExposure(BT::TreeNode& node);

private:
	static CollimatedLightTube4Recipe* self;
};

inline void registernode(BT::BehaviorTreeFactory& factory)
{
	CollimatedLightTube4Recipe* obj = (CollimatedLightTube4Recipe*)ObjectManager::getInstance()->getObject("CollimatedLightTube4Recipe");
	if (!obj)
	{
		throw BT::RuntimeError("CollimatedLightTube4Recipe object not found !");
	}

	factory.registerSimpleAction(
		"CLT_Connect",
		[=](BT::TreeNode& node)-> BT::NodeStatus
		{
			return obj->CLT_Connect();
		},
		{});

	factory.registerSimpleAction(
		"CLT_Close",
		[=](BT::TreeNode& node)-> BT::NodeStatus
		{
			return obj->CLT_Close();
		});

	factory.registerSimpleAction(
		"CLT_SetExposure",
		[=](BT::TreeNode& node)-> BT::NodeStatus
		{
			return obj->CLT_SetExposure(node);
		},
		{
			BT::InputPort<std::string>("is_auto", "BOOL e.g. 0/1"),
			BT::InputPort<std::string>("exposure", "string")
		});
}
