#include <spill/model/metadata.h>
#include <vector>

using namespace spill::model;
using namespace spill::model::tables;

MetadataTable::MetadataTable(const MetadataTablesHeader& hdr_, const uint8_t *data_, uint32_t rowcount_, bool sorted_) : hdr(hdr_), data(data_), rowcount(rowcount_), rowsize(0), sorted(sorted_)
{
	
}

MetadataTable::~MetadataTable()
{
	
}

void MetadataTable::AddColumn(MetadataTableColumn& column)
{
	columns.push_back(&column);
	rowsize += column.GetSize();
}

MetadataTable *MetadataTable::CreateTable(const MetadataTablesHeader& hdr, int index, const uint8_t *data, uint32_t rowcount, bool sorted)
{
	switch (index) {
	case 0:
		return new Module(hdr, data, rowcount, sorted);
	case 1:
		return new TypeRef(hdr, data, rowcount, sorted);
	case 2:
		return new TypeDef(hdr, data, rowcount, sorted);
	default:
		return NULL;
	}
}

/*
 * Module
 */
Module::Module(const MetadataTablesHeader& hdr, const uint8_t* data, uint32_t rowcount, bool sorted) 
	: MetadataTable(hdr, data, rowcount, sorted),
	Generation(hdr),
	Name(hdr),
	MvID(hdr),
	EncID(hdr),
	EncBaseID(hdr)
{
	AddColumn(Generation);
	AddColumn(Name);
	AddColumn(MvID);
	AddColumn(EncID);
	AddColumn(EncBaseID);
}

Module::~Module()
{
	
}

/*
 * TypeRef
 */
TypeRef::TypeRef(const MetadataTablesHeader& hdr, const uint8_t* data, uint32_t rowcount, bool sorted) 
	: MetadataTable(hdr, data, rowcount, sorted),
	ResolutionScope(hdr),
	TypeName(hdr),
	TypeNamespace(hdr)
{
	AddColumn(ResolutionScope);
	AddColumn(TypeName);
	AddColumn(TypeNamespace);
}

TypeRef::~TypeRef()
{
	
}

/*
 * TypeDef
 */
TypeDef::TypeDef(const MetadataTablesHeader& hdr, const uint8_t* data, uint32_t rowcount, bool sorted) 
	: MetadataTable(hdr, data, rowcount, sorted),
	Flags(hdr),
	TypeName(hdr),
	TypeNamespace(hdr),
	Extends(hdr),
	FieldList(hdr, 0),
	MethodList(hdr, 0)
{
	AddColumn(Flags);
	AddColumn(TypeName);
	AddColumn(TypeNamespace);
	AddColumn(Extends);
	AddColumn(FieldList);
	AddColumn(MethodList);
}

TypeDef::~TypeDef()
{
	
}
