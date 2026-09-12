#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <format>
#include <stdexcept>
#include "bytearray.h"

#define STATE_SIZE 232

struct Region {
    const char* name;
    const char* elf;
    uint32_t version;
    uint32_t state_addr;
    std::vector<uint32_t> hook_sites;
};

const std::vector<Region> REGIONS = {
    {"4.3U", "hook_US.elf", 513, 0x81359EE0, {0x81334D88, 0x81335284, 0x813354D0, 0x813356EC}},
    {"4.3E", "hook_EU.elf", 514, 0x81359F88, {0x81334DF8, 0x813352F4, 0x81335540, 0x8133575C}},
    {"4.3J", "hook_JP.elf", 512, 0x813593A0, {0x81334268, 0x81334764, 0x813349B0, 0x81334BCC}}
};

struct Section {
    uint32_t name, type, flags, addr, offset;
    uint32_t size, link, info, align, entsize;
};

class ElfStuff {
    public:
        std::vector<uint8_t> data;
        std::vector<Section> sections;
        std::vector<std::string> section_names;
        Section symtab{}, strtab{};

        ElfStuff(ByteArray* b) : data(b->buf, b->buf + b->size) {
            if (data[0] != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') throw std::invalid_argument("not an ELF file");
            if (data[5] != 2) throw std::invalid_argument("ELF is not big endian");
            uint32_t shoff = u32(0x20);
            uint16_t shentsize = u16(0x2e);
            uint16_t shnum = u16(0x30);
            uint16_t shstrndx = u16(0x32);
            for (int i = 0; i < shnum; ++i) {
                size_t off = shoff + i * shentsize;
                sections.push_back({u32(off), u32(off + 4), u32(off + 8), u32(off + 12), u32(off + 16), u32(off + 20), u32(off + 24), u32(off + 28), u32(off + 32), u32(off + 36)});
            }
            auto& shstr = sections[shstrndx];
            for (auto& s : sections) section_names.push_back(str(shstr.offset, s.name));
            auto it = std::find_if(sections.begin(), sections.end(), [](const Section& s) {return s.type == 2;});
            if (it == sections.end()) throw std::invalid_argument("ELF has no symbol table");
            symtab = *it;
            strtab = sections[symtab.link];
        }

        uint16_t u16(size_t o) const {
            return (data[o] << 8) | data[o + 1];
        }

        uint32_t u32(size_t o) const {
            return (uint32_t(data[o]) << 24) | (uint32_t(data[o + 1]) << 16) | (uint32_t(data[o + 2]) << 8) | data[o + 3];
        }

        std::string str(uint32_t base, uint32_t off) const {
            size_t start = base + off;
            size_t end = start;
            while (data[end]) ++end;
            return {reinterpret_cast<const char*>(&data[start]), end - start};
        }
};

std::vector<uint32_t> get_words(const std::vector<uint8_t>& data) {
    std::vector<uint32_t> ret;
    for (size_t i = 0; i < data.size(); i += 4) ret.push_back((uint32_t(data[i]) << 24) | (uint32_t(data[i + 1]) << 16) | (uint32_t(data[i + 2]) << 8) | data[i + 3]);
    return ret;
}


std::size_t emit_patch(std::string& strbuf, std::size_t addr, const std::vector<uint8_t>& elf_data) {
    auto words = get_words(elf_data);
    std::size_t n = 0;
    for (size_t i = 0; i < words.size(); i += 8) {
        size_t end = std::min(i + 8, words.size());
        strbuf += std::format("offset=0x{:08x}\n", addr + i * 4);
        strbuf += "patch=";
        for (size_t j = i; j < end; ++j) {
            if (j != i) strbuf += ",";
            strbuf += std::format("0x{:08x}", words[j]);
        }
        strbuf += "\n";
        ++n;
    }
    return n;
}

void emit_region(std::ofstream& out, const Region& r) {
    ByteArray* raw = file_to_byte_array(r.elf);
    if (!raw) throw std::runtime_error(std::string("could not read ") + r.elf);
    ElfStuff e(raw);
    std::unordered_map<std::string, Section> sec;
    for (size_t i = 0; i < e.section_names.size(); ++i) sec[e.section_names[i]] = e.sections[i];
    auto text = sec[".text"];
    auto rodata = sec[".rodata"];
    std::vector<uint8_t> tb(e.data.begin() + text.offset, e.data.begin() + text.offset + text.size);
    std::vector<uint8_t> rb(e.data.begin() + rodata.offset, e.data.begin() + rodata.offset + rodata.size);
    std::size_t pair_cnt = 0;
    std::string tmp = "";
    pair_cnt += emit_patch(tmp, text.addr, tb);
    pair_cnt += emit_patch(tmp, rodata.addr, rb);
    pair_cnt += emit_patch(tmp, r.state_addr, std::vector<uint8_t>(STATE_SIZE, 0));
    out << "[GameCube Controller Support]\n";
    out << "minversion=" << r.version << "\n";
    out << "maxversion=" << r.version << "\n";
    out << std::format("amount={}\n", pair_cnt + r.hook_sites.size());
    out << tmp;
    for (auto& site : r.hook_sites) {
        uint32_t bl = 0x48000001 | ((text.addr - site) & 0x03FFFFFC);
        out << std::format("offset={:#010x}\n", site);
        out << std::format("patch=0x{:08x}\n", bl);
    }
    cleanup_bytearray(&raw);
    std::cout << r.name << " (v" << r.version << "): entry 0x" << std::format("{:08x}", text.addr) << std::endl;
}

int main(void) {
    std::ofstream out("gcn.ini");
    try {
        for (auto& r : REGIONS) {
            emit_region(out, r);
        }
    } 
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    out.close();
    std::cout << "patch created" << std::endl;
    return 0;
}
