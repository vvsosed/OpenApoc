#include "game/resources/vehiclefactory.h"

#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "game/tileview/voxel.h"

namespace OpenApoc {

VehicleFactory::VehicleFactory(Framework &fw)
	: fw(fw)
{

}

VehicleFactory::~VehicleFactory()
{

}

std::map<Vehicle::Direction, Vec3<float>> directionsToVec =
{
	{Vehicle::Direction::N,  { 0,-1, 0}}, 
	{Vehicle::Direction::NE, { 1,-1, 0}}, 
	{Vehicle::Direction::E,  { 1, 0, 0}}, 
	{Vehicle::Direction::SE, { 1, 1, 0}}, 
	{Vehicle::Direction::S,  { 0, 1, 0}}, 
	{Vehicle::Direction::SW, {-1, 1, 0}}, 
	{Vehicle::Direction::W,  {-1, 0, 0}}, 
	{Vehicle::Direction::NW, {-1,-1, 0}}, 
};

static std::map<Vehicle::Direction, std::shared_ptr<Image> >
parseDirectionalSprites(Framework &fw, tinyxml2::XMLElement *root)
{
	std::map<Vehicle::Direction, std::shared_ptr<Image> > sprites;

	for (tinyxml2::XMLElement* node = root->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		UString name = node->Name();
		Vehicle::Direction dir;
		if (name == "N")
			dir = Vehicle::Direction::N;
		else if (name == "NNE")
			dir = Vehicle::Direction::NNE;
		else if (name == "NE")
			dir = Vehicle::Direction::NE;
		else if (name == "NEE")
			dir = Vehicle::Direction::NEE;
		else if (name == "E")
			dir = Vehicle::Direction::E;
		else if (name == "SEE")
			dir = Vehicle::Direction::SEE;
		else if (name == "SE")
			dir = Vehicle::Direction::SE;
		else if (name == "SSE")
			dir = Vehicle::Direction::SSE;
		else if (name == "S")
			dir = Vehicle::Direction::S;
		else if (name == "SSW")
			dir = Vehicle::Direction::SSW;
		else if (name == "SW")
			dir = Vehicle::Direction::SW;
		else if (name == "SWW")
			dir = Vehicle::Direction::SWW;
		else if (name == "W")
			dir = Vehicle::Direction::W;
		else if (name == "NWW")
			dir = Vehicle::Direction::NWW;
		else if (name == "NW")
			dir = Vehicle::Direction::NW;
		else if (name == "NNW")
			dir = Vehicle::Direction::NNW;
		else
		{
			LogError("Unknown sprite direction \"%s\"", name.str().c_str());
			continue;
		}

		UString spriteName = node->GetText();
		LogInfo("Loading image \"%s\"", spriteName.str().c_str());
		if (sprites[dir])
			LogWarning("Replacing directional sprite");
		auto sprite = fw.gamecore->GetImage(spriteName);

		if (!sprite)
			LogError("Failed to load directional sprite");
		sprites[dir] = sprite;
	}
	return sprites;
}

void
VehicleFactory::ParseVehicleDefinition(tinyxml2::XMLElement *root)
{
	VehicleDefinition def;
	def.name = root->Attribute("id");

	UString type = root->Attribute("type");

	if (type == "flying")
		def.type = Vehicle::Type::Flying;
	else if (type == "ground")
		def.type = Vehicle::Type::Ground;
	else
	{
		LogError("Unknown vehicle type \"%s\"", type.str().c_str());
		return;
	}

	def.size.x = root->FloatAttribute("sizeX");
	def.size.y = root->FloatAttribute("sizeY");
	def.size.z = root->FloatAttribute("sizeZ");

	for (tinyxml2::XMLElement* node = root->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		UString tag = node->Name();
		if (tag == "flat")
		{
			def.sprites[Vehicle::Banking::Flat] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "ascending")
		{
			def.sprites[Vehicle::Banking::Ascending] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "decending")
		{
			def.sprites[Vehicle::Banking::Decending] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "banking_left")
		{
			def.sprites[Vehicle::Banking::Left] = parseDirectionalSprites(fw, node);
		}
		else if (tag == "banking_right")
		{
			def.sprites[Vehicle::Banking::Right] = parseDirectionalSprites(fw, node);
		}
		else
		{
			LogError("Unknown vehicle tag \"%s\"", tag.str().c_str());
			continue;
		}
	}
	//Push all directional sprites into 'directional vector' space
	//FIXME: How to do banking left/right?
	for (auto &s : def.sprites[Vehicle::Banking::Flat])
	{
		Vec3<float> v = directionsToVec[s.first];
		v = glm::normalize(v);
		def.directionalSprites.emplace_back(v, s.second);
	}
	for (auto &s : def.sprites[Vehicle::Banking::Ascending])
	{
		Vec3<float> v = directionsToVec[s.first];
		v.z = 1;
		v = glm::normalize(v);
		def.directionalSprites.emplace_back(v, s.second);
	}
	for (auto &s : def.sprites[Vehicle::Banking::Decending])
	{
		Vec3<float> v = directionsToVec[s.first];
		v.z = -1;
		v = glm::normalize(v);
		def.directionalSprites.emplace_back(v, s.second);
	}

	if (!def.voxelMap)
	{
		static std::weak_ptr<VoxelMap> stubVoxelMap;
		LogWarning("Using stub voxel map for vehicle \"%s\"", def.name.str().c_str());
		def.voxelMap = stubVoxelMap.lock();
		if (!def.voxelMap)
		{
			def.voxelMap = std::make_shared<VoxelMap>(Vec3<int>{32,32,16});
			stubVoxelMap = def.voxelMap;
		}
	}

	this->defs.emplace(def.name, def);

}

std::shared_ptr<Vehicle>
VehicleFactory::create(const UString name, Organisation &owner)
{
	auto &def = this->defs[name];
	auto v = std::shared_ptr<Vehicle>(new Vehicle(def, owner));


	return v;
}

}; //namespace OpenApoc
