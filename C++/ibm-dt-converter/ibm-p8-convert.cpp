#define ROOT_ADDR 0x1F5C00
#define FILENAME "003000.dat"
#define SECTOR_SZ 1024

#define META_ENTRY_MARK 4
#define META_NUMBER_OF_SECTORS 0x12
#define META_SIZE_IN_SECTORS 0xE
#define META_OFFSET 0x26
#define CATALOG_NAME_LENGTHS 0x40

#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <cstdint>

struct MetaData {  //may be used for more  thorough converter
	uint32_t numberOfSectors;
	uint32_t metadataSize;
	uint32_t offset1;
	uint8_t sectorsCount;
	uint8_t subtype1;
	uint8_t subtype2;
	std::string className;
	std::string indexName;
	std::string indexValue;
	std::string formatName;
	std::string formatValue;
	std::string filename;
};

struct Catalog {  //may be used for more  thorough converter
	uint32_t numberOfSectors;
	uint32_t DOCID;
	uint32_t offset1;
	uint8_t sectorsCount;
	uint8_t subtype1;
	uint8_t subtype2;
	std::string className;
	std::string indexName;
	std::string indexValue;
	std::string formatName;
	std::string formatValue;
	std::string filename;
	uint32_t startPos;
	uint32_t dataLength;
};

uint32_t get16(int index, const std::string &rawdata) {
	unsigned char *raw = (unsigned char*)&rawdata[index];
	return (raw[0] << 8) + raw[1];
}

uint32_t get32(int index, const std::string &rawdata) {
	unsigned char *raw = (unsigned char*)&rawdata[index];
	return (raw[0] << 24) + (raw[1] << 16) + (raw[2] << 8) + raw[3];
}

bool doWork(const std::string &rawdata) {
	uint32_t p = ROOT_ADDR, q = p, d;
	MetaData md;
	Catalog cat;
	for(;;) {
		if (get32(q, rawdata) != META_ENTRY_MARK) break;
		md.numberOfSectors = get32(q + META_NUMBER_OF_SECTORS, rawdata)*SECTOR_SZ;
		md.metadataSize = get32(q + META_SIZE_IN_SECTORS, rawdata)*SECTOR_SZ;
		md.offset1 = get32(q + META_OFFSET, rawdata)*SECTOR_SZ;
		q += md.metadataSize;
		d = md.offset1 + CATALOG_NAME_LENGTHS;
		uint32_t sum = 5;
		for (int i = 0; i < 4; ++i)
			sum += (uint8_t)rawdata[d + i];
		d += sum;
		d += (uint8_t)rawdata[d] + 1;
		d += (uint8_t)rawdata[d] + 2;
		d += (uint8_t)rawdata[d] + 2;
		int pos1 = rawdata.find("name=", d) + 6;
		int pos2 = rawdata.find("\"", pos1);
		cat.filename = rawdata.substr(pos1, pos2 - pos1);
		d = (pos2 & ~3) + 20;
		cat.dataLength = get32(d + 4, rawdata);
		uint32_t fp = get32(d, rawdata)*SECTOR_SZ;
		std::ofstream fo(cat.filename);
		if (md.offset1 != fp) 
			cat.startPos = fp;
		else
			cat.startPos = d + 8;
		fo.write(&rawdata[cat.startPos], cat.dataLength);
		fo.close();
	}
	return true;
}

int main() {
	std::ifstream fi(FILENAME);
	std::string rawdata((std::istreambuf_iterator<char>(fi)), std::istreambuf_iterator<char>());
	doWork(rawdata);
	fi.close();
	return 0;
}

