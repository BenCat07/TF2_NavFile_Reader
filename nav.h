#pragma once

#include "Vector.h"
#include <vector>

class CNavPlace
{
public:
	char m_name[256];
};

enum NavDirType
{
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	NUM_DIRECTIONS
};

enum { MAX_NAV_TEAMS = 2};

/**
* A HidingSpot is a good place for a bot to crouch and wait for enemies
*/
class HidingSpot
{
public:
	enum
	{
		IN_COVER = 0x01,							// in a corner with good hard cover nearby
		GOOD_SNIPER_SPOT = 0x02,							// had at least one decent sniping corridor
		IDEAL_SNIPER_SPOT = 0x04,							// can see either very far, or a large area, or both
		EXPOSED = 0x08							// spot in the open, usually on a ledge or cliff
	};

	bool HasGoodCover(void) const { return (m_flags & IN_COVER) ? true : false; }	// return true if hiding spot in in cover
	bool IsGoodSniperSpot(void) const { return (m_flags & GOOD_SNIPER_SPOT) ? true : false; }
	bool IsIdealSniperSpot(void) const { return (m_flags & IDEAL_SNIPER_SPOT) ? true : false; }
	bool IsExposed(void) const { return (m_flags & EXPOSED) ? true : false; }

	Vector m_pos;											// world coordinates of the spot
	unsigned int m_id;										// this spot's unique ID
	unsigned char m_flags;									// bit flags
};

class CNavArea;
struct NavConnect;

struct AreaBindInfo
{
	union
	{
		CNavArea *area;
		unsigned int id = 0;
	};

	unsigned char attributes;				// VisibilityType
};

enum NavAttributeType
{
	NAV_MESH_INVALID = 0,
	NAV_MESH_CROUCH = 0x0000001, // must crouch to use this node/area
	NAV_MESH_JUMP = 0x0000002, // must jump to traverse this area (only used during generation)
	NAV_MESH_PRECISE = 0x0000004, // do not adjust for obstacles, just move along area
	NAV_MESH_NO_JUMP = 0x0000008, // inhibit discontinuity jumping
	NAV_MESH_STOP = 0x0000010, // must stop when entering this area
	NAV_MESH_RUN = 0x0000020, // must run to traverse this area
	NAV_MESH_WALK = 0x0000040, // must walk to traverse this area
	NAV_MESH_AVOID = 0x0000080, // avoid this area unless alternatives are too dangerous
	NAV_MESH_TRANSIENT = 0x0000100, // area may become blocked, and should be periodically checked
	NAV_MESH_DONT_HIDE = 0x0000200, // area should not be considered for hiding spot generation
	NAV_MESH_STAND = 0x0000400, // bots hiding in this area should stand
	NAV_MESH_NO_HOSTAGES = 0x0000800, // hostages shouldn't use this area
	NAV_MESH_STAIRS = 0x0001000, // this area represents stairs, do not attempt to climb or jump them - just walk up
	NAV_MESH_NO_MERGE = 0x0002000, // don't merge this area with adjacent areas
	NAV_MESH_OBSTACLE_TOP = 0x0004000, // this nav area is the climb point on the tip of an obstacle
	NAV_MESH_CLIFF = 0x0008000, // this nav area is adjacent to a drop of at least CliffHeight

	NAV_MESH_FIRST_CUSTOM = 0x00010000, // apps may define custom app-specific bits starting with this value
	NAV_MESH_LAST_CUSTOM = 0x04000000, // apps must not define custom app-specific bits higher than with this value

	NAV_MESH_HAS_ELEVATOR = 0x40000000, // area is in an elevator's path
	NAV_MESH_NAV_BLOCKER = 0x80000000, // area is blocked by nav blocker ( Alas, needed to hijack a bit in the attributes to get within a cache line [7/24/2008 tom])
};


enum NavTraverseType
{
	// NOTE: First 4 directions MUST match NavDirType
	GO_NORTH = 0,
	GO_EAST,
	GO_SOUTH,
	GO_WEST,

	GO_LADDER_UP,
	GO_LADDER_DOWN,
	GO_JUMP,
	GO_ELEVATOR_UP,
	GO_ELEVATOR_DOWN,

	NUM_TRAVERSE_TYPES
};

enum NavCornerType
{
	NORTH_WEST = 0,
	NORTH_EAST = 1,
	SOUTH_EAST = 2,
	SOUTH_WEST = 3,

	NUM_CORNERS
};

enum NavRelativeDirType
{
	FORWARD = 0,
	RIGHT,
	BACKWARD,
	LEFT,
	UP,
	DOWN,

	NUM_RELATIVE_DIRECTIONS
};

enum LadderDirectionType
{
	LADDER_UP = 0,
	LADDER_DOWN,

	NUM_LADDER_DIRECTIONS
};

class CNavArea
{
public:
	uint32_t m_id;
	int32_t m_attributeFlags;
	Vector m_nwCorner;
	Vector m_seCorner;
	Vector m_center;
	float m_invDzCorners;
	float m_invDxCorners;
	float m_neY;
	float m_swY;
	float m_minZ;
	float m_maxZ;
	std::vector<NavConnect> m_connections;
	std::vector<HidingSpot> m_hidingSpots;
	uint32_t m_encounterSpotCount;
	uint16_t m_indexType;
	float m_earliestOccupyTime[MAX_NAV_TEAMS];
	float m_lightIntensity[NUM_CORNERS];
	uint32_t m_visibleAreaCount;
	uint32_t m_inheritVisibilityFrom;
	std::vector<AreaBindInfo> m_potentiallyVisibleAreas;

	// Check if the given point is overlapping the area
	// @return True if 'pos' is within 2D extents of area.
	bool IsOverlapping(const Vector &vecPos, float flTolerance = 0)
	{
		if (vecPos.x + flTolerance < this->m_nwCorner.x)
			return false;

		if (vecPos.x - flTolerance > this->m_seCorner.x)
			return false;

		if (vecPos.y + flTolerance < this->m_nwCorner.y)
			return false;

		if (vecPos.y - flTolerance > this->m_seCorner.y)
			return false;

		return true;
	}

	//Check if the point is within the 3D bounds of this area
	bool Contains(Vector &vecPoint)
	{
		if (!IsOverlapping(vecPoint))
			return false;

		if (vecPoint.z > m_maxZ)
			return false;

		if (vecPoint.z < m_minZ)
			return false;

		return true;
	}
};

struct NavConnect
{
	NavConnect()
	{
		id = 0;
		length = -1;
	}

	union
	{
		unsigned int id;
		CNavArea *area;
	};

	mutable float length;

	bool operator==(const NavConnect &other) const
	{
		return (area == other.area) ? true : false;
	}
};

struct SpotOrder
{
	float t; // parametric distance along ray where this spot first has LOS to our path

	union
	{
		HidingSpot* spot; // the spot to look at
		unsigned int id; // spot ID for save/load
	};
};

/**
* This struct stores possible path segments thru a CNavArea, and the dangerous spots
* to look at as we traverse that path segment.
*/
struct SpotEncounter
{
	NavConnect from;
	NavDirType fromDir;
	NavConnect to;
	NavDirType toDir;
	//Ray path;									// the path segment
	std::vector<SpotOrder> spots;						// list of spots to look at, in order of occurrence
};