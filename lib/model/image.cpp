#include <spill/model/image.h>
#include <spill/model/metadata.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>

using namespace spill::model;

Image::Image() : loaded(false), tilde_stream(NULL), strings_stream(NULL), us_stream(NULL), blob_stream(NULL), guid_stream(NULL)
{
	bzero(tables, sizeof(*tables) * sizeof(tables[0]));
}

Image::~Image()
{
	// Always try to delete streams here, in case stream objects were created, but the image failed to
	// load later on.
	if (tilde_stream)
		delete tilde_stream;
	if (strings_stream)
		delete strings_stream;
	if (us_stream)
		delete us_stream;
	if (blob_stream)
		delete blob_stream;
	if (guid_stream)
		delete guid_stream;
	
	// Same for metadata tables.
	for (int i = 0; i < 64; i++) {
		if (tables[i])
			delete tables[i];
	}
	
	// If an image was loaded, unmap the data.
	if (loaded) {
		munmap((void *)data, size);
	}
}

bool Image::VerifyImage()
{
	// First, check the file-size.  Obviously, sizeof(ImageDosHeader) as a limit is
	// pretty ridiculous, but at least we can verify the start of the file using a
	// known structure.
	if (size < sizeof(ImageDosHeader)) {
		LOG(ERROR) << "file size too small.";
		return false;
	}

	// Now, locate the DOS header and check the magic number.
	const ImageDosHeader *dos_header = (const ImageDosHeader *) (data);

	if (dos_header->MagicBytes[0] != 'M' || dos_header->MagicBytes[1] != 'Z') {
		LOG(ERROR) << "dos header magic number incorrect.";
		return false;
	}

	// Make sure we don't point past the end of the file.
	if (dos_header->NTHeaderOffset > size) {
		LOG(ERROR) << "nt header past end-of-file.";
		return false;
	}

	// Locate the NT headers, and check the magic number.
	const ImageNtHeaders *nt_headers = (const ImageNtHeaders *) (data + dos_header->NTHeaderOffset);

	if (nt_headers->Signature != 0x00004550) {
		LOG(ERROR) << "pe signature incorrect.";
		return false;
	}

	// Make sure there's a CLI data directory.
	if (nt_headers->OptionalHeader.DataDirectory[14].Size == 0) {
		LOG(ERROR) << "cli data directory does not exist.";
		return false;
	}

	// Find the text section.
	const ImageSectionHeader *section_headers = (const ImageSectionHeader *) ((const uint8_t *) & nt_headers->OptionalHeader + nt_headers->FileHeader.SizeOfOptionalHeader);
	unsigned int text_section_index = -1;

	for (unsigned int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
		LOG(DEBUG) << "analysing section: " << section_headers[i].Name;
		if (strcmp((const char *) section_headers[i].Name, ".text") == 0) {
			text_section_index = i;
			LOG(DEBUG) << "found .text section at index " << text_section_index;
			break;
		}
	}

	if (text_section_index < 0) {
		LOG(ERROR) << "unable to locate .text section";
		return false;
	}

	const ImageSectionHeader *text_section = &section_headers[text_section_index];

	// Find the CLI header.
	cli_header = (const ImageCor20Header *) RVAToAddress(text_section, nt_headers->OptionalHeader.DataDirectory[14].VirtualAddress);
	LOG(DEBUG) << "cli header size=" << std::hex << cli_header->cb
		<< ", version major=" << cli_header->MajorRuntimeVersion
		<< ", minor=" << cli_header->MinorRuntimeVersion;

	// Find the Metadata header.
	const uint8_t *metadata = RVAToAddress(text_section, cli_header->Metadata.VirtualAddress);
	uint32_t metadata_signature = *(const uint32_t *) metadata;

	// Check the metadata header signature.
	if (metadata_signature != 0x424a5342) {
		LOG(ERROR) << "metadata signature incorrect.";
		return false;
	}

	uint16_t metadata_vs_length = *(const uint16_t *) (metadata + 12);
	const char *metadata_vs = (const char *) (metadata + 14);

	LOG(DEBUG) << "metadata version string " << std::string(metadata_vs, (size_t) metadata_vs_length);

	// Determine the number of metadata streams present, and ensure > 0.
	const uint16_t *number_of_streams = (const uint16_t *) ((const uint8_t *) metadata_vs + metadata_vs_length + 4 - (metadata_vs_length % 4));
	
	if (*number_of_streams == 0) {
		LOG(ERROR) << "no metadata streams present.";
		return false;
	}

	// Load the streams vector with stream information.
	const uint8_t *stream_headers = (const uint8_t *) number_of_streams + 2;
	for (unsigned int i = 0; i < *number_of_streams; i++) {
		ImageStream *stream = new ImageStream();

		uint32_t offset = *(const uint32_t *) stream_headers;
		uint32_t size = *(const uint32_t *) (stream_headers + 4);
		const char *name = (const char *) stream_headers + 8;

		stream->data = (uint8_t *) metadata + offset;
		stream->size = (unsigned long) size;
		stream->name = std::string(name);

		stream_headers += 8 + strlen(name) + 4 - (strlen(name) % 4);

		if (stream->name == "#~") {
			tilde_stream = stream;
		} else if (stream->name == "#Strings") {
			strings_stream = stream;
		} else if (stream->name == "#US") {
			us_stream = stream;
		} else if (stream->name == "#GUID") {
			guid_stream = stream;
		} else if (stream->name == "#Blob") {
			blob_stream = stream;
		} else {
			LOG(WARNING) << "unexpected stream detected: " << stream;
			delete stream;
		}
	}
	
	if (tilde_stream == NULL) {
		LOG(ERROR) << "missing stream: #~";
		return false;
	}
	
	if (strings_stream == NULL) {
		LOG(ERROR) << "missing stream: #Strings";
		return false;
	}

	return true;
}

bool Image::LoadTables()
{
	assert(tilde_stream);
	
	const MetadataTablesHeader *metadata_tables_header = (const MetadataTablesHeader *)tilde_stream->data;
	const uint32_t *row_count = (const uint32_t *)(tilde_stream->data + sizeof(MetadataTablesHeader));
	
	if (metadata_tables_header->Reserved0 != 0) {
		LOG(ERROR) << "invalid metadata tables header";
		return false;
	}
	
	for (int index = 0; index < 64; index++) {
		int bit = 1 << index;
		if ((metadata_tables_header->ValidTables & bit) == 0) {
			continue;
		}
		
		LOG(DEBUG) << "table " << index << " is present with " << *row_count << " rows.";
		
		tables::MetadataTable *table = tables::MetadataTable::CreateTable(*metadata_tables_header, index, NULL, *row_count, (metadata_tables_header->SortedTables & bit) != 0);
		if (table == NULL) {
			LOG(ERROR) << "table creation failed for " << index;
			return false;
		}
		
		LOG(DEBUG) << "table size=" << table->GetRowSize();
		
		tables[index] = table;
		row_count++;
	}
	
	return true;
}

bool Image::LoadFile(std::string filename)
{
	assert(!loaded);

	// Open the input file for reading.
	int fd = open(filename.c_str(), O_RDONLY);
	if (fd < 0) {
		LOG(ERROR) << "unable to open file '" << filename << "'.";
		return false;
	}

	// Stat the file, so we know how big it is.
	struct stat stat;
	if (fstat(fd, &stat) < 0) {
		LOG(ERROR) << "unable to stat file.";
		close(fd);
		return false;
	}

	// Store the size, and attempt to map the file into memory.
	size = (unsigned long) stat.st_size;
	data = (uint8_t *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

	// We're done with the file-descriptor at this point, regardless of whether or not
	// the mmap was successful.
	close(fd);

	// Now, check the result of the mmap.
	if (data == MAP_FAILED) {
		LOG(ERROR) << "unable to map file into memory.";
		return false;
	}

	// Verify the image.
	if (!VerifyImage()) {
		LOG(ERROR) << "image verification failed.";
		munmap(data, size);
		return false;
	}
	
	// Load metadata tables.
	if (!LoadTables()) {
		LOG(ERROR) << "loading metadata tables failed.";
		munmap(data, size);
		return false;
	}

	LOG(DEBUG) << "assembly image loaded.";

	// Mark the image as loaded.
	loaded = true;
	return true;
}
