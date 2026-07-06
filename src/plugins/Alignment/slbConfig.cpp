#include "slbConfig.h"

bool slbConfig::Load(const char* path)
{
	m_path = path;
	std::ifstream ifs(path, std::fstream::in);

	if (ifs.fail())
	{
		return false;
	}

	ifs >> m_JsonControl;
	return true;
}

slbConfigInfo slbConfig::GetSlbConfigInfo()
{
	slbConfigInfo m_slbConfigInfo;
	m_slbConfigInfo.slb_LoadImageXYZPosition.x = m_JsonControl["SLB_LoadImageXYZPosition"]["X"].get<double>();
	m_slbConfigInfo.slb_LoadImageXYZPosition.y = m_JsonControl["SLB_LoadImageXYZPosition"]["Y"].get<double>();
	m_slbConfigInfo.slb_LoadImageXYZPosition.z = m_JsonControl["SLB_LoadImageXYZPosition"]["Z"].get<double>();

	m_slbConfigInfo.slb_ImagingXYZPosition.x = m_JsonControl["SLB_ImagingXYZPosition"]["X"].get<double>();
	m_slbConfigInfo.slb_ImagingXYZPosition.y = m_JsonControl["SLB_ImagingXYZPosition"]["Y"].get<double>();
	m_slbConfigInfo.slb_ImagingXYZPosition.z = m_JsonControl["SLB_ImagingXYZPosition"]["Z"].get<double>();

	m_slbConfigInfo.slb_DutXYZPosition.x = m_JsonControl["SLB_DutXYZPosition"]["X"].get<double>();
	m_slbConfigInfo.slb_DutXYZPosition.y = m_JsonControl["SLB_DutXYZPosition"]["Y"].get<double>();
	m_slbConfigInfo.slb_DutXYZPosition.z = m_JsonControl["SLB_DutXYZPosition"]["Z"].get<double>();

	m_slbConfigInfo.slb_projectionTiptilt.dx = m_JsonControl["SLB_ProjectionTiptilt"]["dX"].get<double>();
	m_slbConfigInfo.slb_projectionTiptilt.dy = m_JsonControl["SLB_ProjectionTiptilt"]["dY"].get<double>();

	m_slbConfigInfo.slb_imagingTiptilt.dx = m_JsonControl["SLB_ImagingTiptilt"]["dX"].get<double>();
	m_slbConfigInfo.slb_imagingTiptilt.dy = m_JsonControl["SLB_ImagingTiptilt"]["dY"].get<double>();

	return m_slbConfigInfo;
}

SLB_Limit3D slbConfig::GetSlbLimit3D()
{
	SLB_Limit3D limits;
	limits.min.x = m_JsonControl["SLB_ImageXYZLimit"]["Min"]["X"].get<double>();
	limits.min.y = m_JsonControl["SLB_ImageXYZLimit"]["Min"]["Y"].get<double>();
	limits.min.z = m_JsonControl["SLB_ImageXYZLimit"]["Min"]["Z"].get<double>();
	limits.max.x = m_JsonControl["SLB_ImageXYZLimit"]["Max"]["X"].get<double>();
	limits.max.y = m_JsonControl["SLB_ImageXYZLimit"]["Max"]["Y"].get<double>();
	limits.max.z = m_JsonControl["SLB_ImageXYZLimit"]["Max"]["Z"].get<double>();
	return limits;
}