#pragma once

#include "nav.h"
#include <fstream>

class CNavFile
{
public:
	//Intended to use with engine->GetLevelName() or mapname from server_spawn GameEvent
	//Change it if you get the nav file from elsewhere
	CNavFile(const char* szLevelname)
	{
		if (!szLevelname)
			return;

		m_mapName = std::string("tf/");
		std::string map(szLevelname);

		if (map.find("maps/") == std::string::npos)
			m_mapName.append("maps/");

		m_mapName.append(szLevelname);
		int dotpos = m_mapName.find('.');
		m_mapName = m_mapName.substr(0, dotpos);
		m_mapName.append(".nav");

		std::ifstream fs(m_mapName, std::ios::binary);

		if (!fs.is_open())
		{
			//.nav file does not exist
			return;
		}

		uint32_t magic;
		fs.read((char*) &magic, sizeof(uint32_t));

		if (magic != 0xFEEDFACE)
		{
			//Wrong magic number
			return;
		}

		uint32_t version;
		fs.read((char*)&version, sizeof(uint32_t));

		if (version < 16) //16 is latest for TF2
		{
			//Version is too old
			return;
		}

		uint32_t subVersion;
		fs.read((char*)&subVersion, sizeof(uint32_t));

		if (subVersion != 2) //2 for TF2
		{
			//Not TF2 nav file
			return;
		}

		//We do not really need to check the size
		uint32_t bspSize;
		fs.read((char*)&bspSize, sizeof(uint32_t));
		fs.read((char*)&m_isAnalized, sizeof(unsigned char));
		uint16_t placeCount;
		fs.read((char*)&placeCount, sizeof(uint16_t));

		//TF2 does not use places, but in case they exist
		uint16_t len;
		for (int i = 0; i < placeCount; i++)
		{
			fs.read((char*)&len, sizeof(uint16_t));

			CNavPlace place;

			fs.read((char*) &place.m_name, len);

			m_places.push_back(place);
		}

		fs.read((char*)&m_hasUnnamedAreas, sizeof(unsigned char));

		uint32_t areaCount;
		fs.read((char*)&areaCount, sizeof(uint32_t));
		

		for (size_t i = 0; i < areaCount; i++)
		{
			CNavArea area;
			fs.read((char*)&area.m_id, sizeof(uint32_t));
			fs.read((char*)&area.m_attributeFlags, sizeof(uint32_t));
			fs.read((char*)&area.m_nwCorner, sizeof(Vector));
			fs.read((char*)&area.m_seCorner, sizeof(Vector));
			fs.read((char*)&area.m_neY, sizeof(float));
			fs.read((char*)&area.m_swY, sizeof(float));

			area.m_center[0] = (area.m_nwCorner[0] + area.m_seCorner[0]) / 2.0f;
			area.m_center[1] = (area.m_nwCorner[1] + area.m_seCorner[1]) / 2.0f;
			area.m_center[2] = (area.m_nwCorner[2] + area.m_seCorner[2]) / 2.0f;

			if ((area.m_seCorner.x - area.m_nwCorner.x) > 0.0f && (area.m_seCorner.y - area.m_nwCorner.y) > 0.0f)
			{
				area.m_invDxCorners = 1.0f / (area.m_seCorner.x - area.m_nwCorner.x);
				area.m_invDzCorners = 1.0f / (area.m_seCorner.z - area.m_nwCorner.z);
			}
			else
				area.m_invDxCorners = area.m_invDzCorners = 0.0f;

			//Change the tolerance if you wish
			area.m_minZ = min(area.m_seCorner.z, area.m_nwCorner.z) - 18.f;
			area.m_maxZ = max(area.m_seCorner.z, area.m_nwCorner.z) + 18.f;

			for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
			{
				uint32_t connectionCount;
				fs.read((char*)&connectionCount, sizeof(uint32_t));

				for (size_t j = 0; j < connectionCount; j++)
				{
					NavConnect connect;

					fs.read((char*)&connect.id, sizeof(uint32_t));

					//Connection to the same area?
					if (connect.id == area.m_id)
						continue;

					//Note: If connection directions matter to you, uncomment this
					area.m_connections/*[dir]*/.push_back(connect);
				}
			}

			uint8_t hidingSpotCount;
			fs.read((char*)&hidingSpotCount, sizeof(uint8_t));

			for (size_t j = 0; j < hidingSpotCount; j++)
			{
				HidingSpot spot;
				fs.read((char*)&spot.m_id, sizeof(uint32_t));
				fs.read((char*)&spot.m_pos, sizeof(Vector));
				fs.read((char*)&spot.m_flags, sizeof(unsigned char));

				area.m_hidingSpots.push_back(spot);
			}

			fs.read((char*)&area.m_encounterSpotCount, sizeof(uint32_t));

			for (size_t j = 0; j < area.m_encounterSpotCount; j++)
			{
				SpotEncounter spot;
				fs.read((char*)&spot.from.id, sizeof(uint32_t));
				fs.read((char*)&spot.fromDir, sizeof(unsigned char));
				fs.read((char*)&spot.to.id, sizeof(uint32_t));
				fs.read((char*)&spot.toDir, sizeof(unsigned char));

				unsigned char spotcount;
				fs.read((char*)&spotcount, sizeof(unsigned char));

				for (int s = 0; s < spotcount; ++s)
				{
					SpotOrder order;
					fs.read((char*)&order.id, sizeof(uint32_t));
					fs.read((char*)&order.t, sizeof(unsigned char));
					spot.spots.push_back(order);
				}
			}

			fs.read((char*)&area.m_indexType, sizeof(uint16_t));

			//TF2 does not use ladders either
			for (int dir = 0; dir < NUM_LADDER_DIRECTIONS; dir++)
			{
				uint32_t laddercount;
				fs.read((char*)&laddercount, sizeof(uint32_t));

				for (size_t j = 0; j < laddercount; j++)
				{
					int temp;
					fs.read((char*)&temp, sizeof(uint32_t));
				}
			}

			for (int j = 0; j < MAX_NAV_TEAMS; j++)
				fs.read((char*)&area.m_earliestOccupyTime[j], sizeof(float));
			

			for (int j = 0; j < NUM_CORNERS; ++j)
				fs.read((char*)&area.m_lightIntensity[j], sizeof(float));
			

			fs.read((char*)&area.m_visibleAreaCount, sizeof(uint32_t));

			for (size_t j = 0; j < area.m_visibleAreaCount; ++j)
			{
				AreaBindInfo info;
				fs.read((char*)&info.id, sizeof(uint32_t));
				fs.read((char*)&info.attributes, sizeof(unsigned char));

				area.m_potentiallyVisibleAreas.push_back(info);
			}

			fs.read((char*)&area.m_inheritVisibilityFrom, sizeof(uint32_t));

			//Unknown 4 bytes
			uint32_t unk;
			fs.read((char*)&unk, sizeof(uint32_t));

			m_areas.push_back(area);
		}

		fs.close();

		//Fill connection for every area with their area ptrs instead of IDs
		//This will come in handy in path finding

		for (auto it = m_areas.begin(); it != m_areas.end(); it++)
		{
			CNavArea& area = *it;

			for (auto it2 = area.m_connections.begin(); it2 != area.m_connections.end(); it2++)
			{
				NavConnect& connection = *it2;

				for (auto it3 = m_areas.begin(); it3 != m_areas.end(); it3++)
				{
					CNavArea& connectedarea = *it3;

					if (connection.id == connectedarea.m_id)
					{
						connection.area = &connectedarea;
					}
				}
			}

			//Fill potentially visible areas as well
			for (auto it2 = area.m_potentiallyVisibleAreas.begin(); it2 != area.m_potentiallyVisibleAreas.end(); it2++)
			{
				AreaBindInfo& bindinfo = *it2;

				for (auto it3 = m_areas.begin(); it3 != m_areas.end(); it3++)
				{
					CNavArea& boundarea = *it3;

					if (bindinfo.id == boundarea.m_id)
					{
						bindinfo.area = &boundarea;
					}
				}
			}
		}
		

		m_isOK = true;
	}


	std::string m_mapName;
	bool m_isAnalized;
	std::vector<CNavPlace> m_places;
	bool m_hasUnnamedAreas;
	std::vector<CNavArea> m_areas;
	bool m_isOK = false;
};