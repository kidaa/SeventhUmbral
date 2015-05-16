#include <vector>
#include <assert.h>
#include "ResourceSection.h"
#include "ShaderSection.h"
#include "ModelSection.h"
#include "SkeletonSection.h"
#include "PhbSection.h"
#include "TextureSection.h"

CResourceSection::CResourceSection()
{

}

CResourceSection::~CResourceSection()
{

}

void CResourceSection::Read(Framework::CStream& inputStream)
{
	CBaseSection::Read(inputStream);

	SEDBRES_HEADER header = {};
	inputStream.Read(&header, sizeof(SEDBRES_HEADER));

	std::vector<SEDBRES_ENTRY> entries;
	entries.resize(header.resourceCount);
	inputStream.Read(entries.data(), header.resourceCount * sizeof(SEDBRES_ENTRY));

	uint64 basePosition = inputStream.Tell();

	assert(header.resourceCount >= 2);

	const auto& resourceTypeEntry = entries[header.resourceCount - 2];
	const auto& resourceIdEntry = entries[header.resourceCount - 1];

	std::vector<uint32> resourceTypes;
	resourceTypes.resize(header.resourceCount);
	inputStream.Seek(basePosition + resourceTypeEntry.offset, Framework::STREAM_SEEK_SET);
	inputStream.Read(resourceTypes.data(), sizeof(uint32) * header.resourceCount);

	//Read resource ids
	std::vector<std::string> resourceIds;
	for(unsigned int i = 0; i < header.resourceCount; i++)
	{
		inputStream.Seek(basePosition + resourceIdEntry.offset + (i * 0x10), Framework::STREAM_SEEK_SET);
		resourceIds.push_back(inputStream.ReadString());
	}

	//Read string table
	inputStream.Seek(basePosition + header.stringsOffset, Framework::STREAM_SEEK_SET);
	std::vector<std::string> strings;
	for(unsigned int i = 0; i < header.stringCount; i++)
	{
		strings.push_back(inputStream.ReadString());
	}

	for(unsigned int i = 0; i < header.resourceCount; i++)
	{
		const auto& entry = entries[i];
		if(entry.type == 0 || entry.size == 0) continue;
		uint32 resourceType = resourceTypes[i];
		inputStream.Seek(basePosition + entry.offset, Framework::STREAM_SEEK_SET);
		SectionPtr section;
		switch(resourceType)
		{
		case '\0trb':
		case '\0bin':
			section = std::make_shared<CResourceSection>();
			break;
		case 'sdrb':
			section = std::make_shared<CShaderSection>();
			break;
		case '\0wrb':
			section = std::make_shared<CModelSection>();
			break;
		case '\0skl':
			section = std::make_shared<CSkeletonSection>();
			break;
		case '\0phb':
			section = std::make_shared<CPhbSection>();
			break;
		case '\0txb':
			section = std::make_shared<CTextureSection>();
			break;
		default:
			assert(0);
			break;
		}
		if(section)
		{
			AddChild(section);
			section->Read(inputStream);
			section->SetResourceId(resourceIds[i]);
			section->SetResourcePath(strings[i]);
		}
	}

	inputStream.Seek(basePosition, Framework::STREAM_SEEK_SET);
}
